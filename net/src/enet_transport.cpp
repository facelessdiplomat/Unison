#include <unison/net/enet_transport.hpp>

#include <enet/enet.h>

#include <algorithm>
#include <vector>

namespace unison::net
{

namespace
{

constexpr std::size_t kChannelCount = 2;
constexpr enet_uint8 kReliableChannel = 0;
constexpr enet_uint8 kUnreliableChannel = 1;

tl::unexpected<Error> unavailable(std::string_view why)
{
    return tl::unexpected{Error{ErrorCode::NetworkUnavailable, why}};
}

std::optional<ENetAddress> addressOf(const EnetAddress& address)
{
    ENetAddress parsed{};

    if (enet_address_set_host_ip(&parsed, address.host.c_str()) != 0)
    {
        return std::nullopt;
    }

    parsed.port = address.port;

    return parsed;
}

class EnetShutdown
{
public:
    EnetShutdown() = default;

    EnetShutdown(const EnetShutdown&) = delete;
    EnetShutdown& operator=(const EnetShutdown&) = delete;
    EnetShutdown(EnetShutdown&&) = delete;
    EnetShutdown& operator=(EnetShutdown&&) = delete;

    ~EnetShutdown()
    {
        enet_deinitialize();
    }
};

struct HostDeleter
{
    void operator()(ENetHost* host) const
    {
        enet_host_destroy(host);
    }
};

void sendNow(ENetPeer* connection, Channel channel, std::span<const std::byte> message)
{
    const bool isReliable = channel == Channel::Reliable;
    ENetPacket* packet =
        enet_packet_create(message.data(), message.size(), isReliable ? ENET_PACKET_FLAG_RELIABLE : 0U);

    if (enet_peer_send(connection, isReliable ? kReliableChannel : kUnreliableChannel, packet) != 0)
    {
        enet_packet_destroy(packet);
    }
}

}

struct EnetTransport::Host
{
    struct Waiting
    {
        Channel channel = Channel::Reliable;
        std::vector<std::byte> bytes;
    };

    struct Peer
    {
        PeerId id{};
        ENetPeer* connection = nullptr;
        bool isConnected = false;
        std::vector<Waiting> waiting;
    };

    Host(ENetHost* handle, std::chrono::milliseconds peerTimeout)
        : handle{handle}, peerTimeoutMilliseconds{static_cast<enet_uint32>(peerTimeout.count())}
    {
    }

    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;
    Host(Host&&) = delete;
    Host& operator=(Host&&) = delete;

    ~Host()
    {
        for (const Peer& peer : peers)
        {
            enet_peer_disconnect_now(peer.connection, 0);
        }
    }

    [[nodiscard]] Peer* find(const ENetPeer* connection)
    {
        const auto found = std::ranges::find(peers, connection, &Peer::connection);

        return found != peers.end() ? &*found : nullptr;
    }

    [[nodiscard]] Peer* find(PeerId id)
    {
        const auto found = std::ranges::find(peers, id, &Peer::id);

        return found != peers.end() ? &*found : nullptr;
    }

    PeerId name(ENetPeer* connection, bool isConnected)
    {
        enet_peer_timeout(connection, 0, peerTimeoutMilliseconds, peerTimeoutMilliseconds);
        peers.push_back(Peer{PeerId{++lastPeerId}, connection, isConnected, {}});

        return peers.back().id;
    }

    void markConnected(ENetPeer* connection)
    {
        Peer* peer = find(connection);

        if (peer == nullptr)
        {
            static_cast<void>(name(connection, true));

            return;
        }

        peer->isConnected = true;

        for (const Waiting& message : peer->waiting)
        {
            sendNow(connection, message.channel, message.bytes);
        }

        peer->waiting.clear();
        enet_host_flush(handle.get());
    }

    std::optional<PeerId> forget(const ENetPeer* connection)
    {
        const Peer* peer = find(connection);

        if (peer == nullptr)
        {
            return std::nullopt;
        }

        const PeerId gone = peer->id;
        std::erase_if(peers, [connection](const Peer& known) { return known.connection == connection; });

        return gone;
    }

    EnetShutdown shutdown;
    std::unique_ptr<ENetHost, HostDeleter> handle;
    enet_uint32 peerTimeoutMilliseconds;
    std::vector<Peer> peers;
    std::uint32_t lastPeerId = 0;
};

tl::expected<std::unique_ptr<EnetTransport>, Error>
EnetTransport::listen(const EnetAddress& at, std::size_t maxPeers, std::chrono::milliseconds peerTimeout)
{
    const std::optional<ENetAddress> address = addressOf(at);

    if (!address.has_value())
    {
        return unavailable("the address to listen on is no address");
    }

    if (enet_initialize() != 0)
    {
        return unavailable("the network could not be started");
    }

    ENetHost* handle = enet_host_create(&*address, maxPeers, kChannelCount, 0, 0);

    if (handle == nullptr)
    {
        enet_deinitialize();

        return unavailable("the address cannot be listened on");
    }

    return std::make_unique<EnetTransport>(Passkey{}, std::make_unique<Host>(handle, peerTimeout));
}

tl::expected<EnetConnection, Error> EnetTransport::connect(const EnetAddress& to,
                                                           const std::optional<EnetAddress>& from,
                                                           std::chrono::milliseconds peerTimeout)
{
    const std::optional<ENetAddress> server = addressOf(to);
    const std::optional<ENetAddress> local = from.has_value() ? addressOf(*from) : std::nullopt;

    if (!server.has_value() || (from.has_value() && !local.has_value()))
    {
        return unavailable("the address to connect to or from is no address");
    }

    if (enet_initialize() != 0)
    {
        return unavailable("the network could not be started");
    }

    ENetHost* handle = enet_host_create(local.has_value() ? &*local : nullptr, 1, kChannelCount, 0, 0);

    if (handle == nullptr)
    {
        enet_deinitialize();

        return unavailable("the address to connect from cannot be used");
    }

    auto transport = std::make_unique<EnetTransport>(Passkey{}, std::make_unique<Host>(handle, peerTimeout));
    ENetPeer* connection = enet_host_connect(handle, &*server, kChannelCount, 0);

    if (connection == nullptr)
    {
        return unavailable("the connection could not be started");
    }

    const PeerId serverId = transport->host->name(connection, false);

    return EnetConnection{std::move(transport), serverId};
}

EnetTransport::EnetTransport(Passkey, std::unique_ptr<Host> host) : host{std::move(host)}
{
}

EnetTransport::~EnetTransport() = default;

std::uint16_t EnetTransport::port() const
{
    return host->handle->address.port;
}

void EnetTransport::send(PeerId to, Channel channel, std::span<const std::byte> message)
{
    Host::Peer* peer = host->find(to);

    if (peer == nullptr)
    {
        return;
    }

    if (!peer->isConnected)
    {
        peer->waiting.push_back(Host::Waiting{channel, {message.begin(), message.end()}});

        return;
    }

    sendNow(peer->connection, channel, message);
    enet_host_flush(host->handle.get());
}

void EnetTransport::poll(IMessageReceiver& receiver)
{
    ENetEvent event{};

    while (enet_host_service(host->handle.get(), &event, 0) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
                host->markConnected(event.peer);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                if (const std::optional<PeerId> gone = host->forget(event.peer))
                {
                    receiver.peerLeft(*gone);
                }

                break;
            case ENET_EVENT_TYPE_RECEIVE:
            {
                const Host::Peer* peer = host->find(event.peer);

                receiver.receive(peer != nullptr ? peer->id : PeerId{},
                                 event.channelID == kReliableChannel ? Channel::Reliable : Channel::Unreliable,
                                 std::as_bytes(std::span{event.packet->data, event.packet->dataLength}));
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

}
