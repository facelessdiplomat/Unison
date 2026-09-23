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

}

struct EnetTransport::Host
{
    struct Peer
    {
        PeerId id{};
        ENetPeer* connection = nullptr;
    };

    explicit Host(ENetHost* handle) : handle{handle}
    {
    }

    [[nodiscard]] PeerId idOf(const ENetPeer* connection) const
    {
        const auto found = std::ranges::find(peers, connection, &Peer::connection);

        return found != peers.end() ? found->id : PeerId{};
    }

    [[nodiscard]] ENetPeer* connectionOf(PeerId id) const
    {
        const auto found = std::ranges::find(peers, id, &Peer::id);

        return found != peers.end() ? found->connection : nullptr;
    }

    void admit(ENetPeer* connection)
    {
        peers.push_back(Peer{PeerId{++lastPeerId}, connection});
    }

    void forget(const ENetPeer* connection)
    {
        std::erase_if(peers, [connection](const Peer& peer) { return peer.connection == connection; });
    }

    EnetShutdown shutdown;
    std::unique_ptr<ENetHost, HostDeleter> handle;
    std::vector<Peer> peers;
    std::uint32_t lastPeerId = 0;
};

tl::expected<std::unique_ptr<EnetTransport>, Error> EnetTransport::listen(const EnetAddress& at, std::size_t maxPeers)
{
    if (enet_initialize() != 0)
    {
        return unavailable("the network could not be started");
    }

    ENetAddress address{};

    if (enet_address_set_host_ip(&address, at.host.c_str()) != 0)
    {
        enet_deinitialize();

        return unavailable("the address to listen on is no address");
    }

    address.port = at.port;
    ENetHost* handle = enet_host_create(&address, maxPeers, kChannelCount, 0, 0);

    if (handle == nullptr)
    {
        enet_deinitialize();

        return unavailable("the address cannot be listened on");
    }

    return std::make_unique<EnetTransport>(Passkey{}, std::make_unique<Host>(handle));
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
    ENetPeer* connection = host->connectionOf(to);

    if (connection == nullptr)
    {
        return;
    }

    const bool isReliable = channel == Channel::Reliable;
    ENetPacket* packet =
        enet_packet_create(message.data(), message.size(), isReliable ? ENET_PACKET_FLAG_RELIABLE : 0U);

    if (enet_peer_send(connection, isReliable ? kReliableChannel : kUnreliableChannel, packet) != 0)
    {
        enet_packet_destroy(packet);

        return;
    }

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
                host->admit(event.peer);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                host->forget(event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                receiver.receive(host->idOf(event.peer),
                                 event.channelID == kReliableChannel ? Channel::Reliable : Channel::Unreliable,
                                 std::as_bytes(std::span{event.packet->data, event.packet->dataLength}));
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

}
