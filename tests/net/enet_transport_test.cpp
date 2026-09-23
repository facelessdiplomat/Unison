#include <unison/net/enet_transport.hpp>

#include <catch2/catch_test_macros.hpp>

#include <enet/enet.h>

#include <support/fatal_handler_probe.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace
{

constexpr std::size_t kPeers = 4;
constexpr auto kPatience = std::chrono::seconds{2};

struct Received
{
    unison::net::PeerId from{};
    unison::net::Channel channel = unison::net::Channel::Reliable;
    std::vector<std::byte> bytes;
};

class Inbox final : public unison::net::IMessageReceiver
{
public:
    void receive(unison::net::PeerId from, unison::net::Channel channel, std::span<const std::byte> message) override
    {
        messages.push_back(Received{from, channel, {message.begin(), message.end()}});
    }

    void peerLeft(unison::net::PeerId peer) override
    {
        departures.push_back(peer);
    }

    std::vector<Received> messages;
    std::vector<unison::net::PeerId> departures;
};

std::unique_ptr<unison::net::EnetTransport> listeningServer()
{
    auto server = unison::net::EnetTransport::listen(unison::net::EnetAddress{"127.0.0.1", 0}, kPeers);

    REQUIRE(server.has_value());

    return std::move(*server);
}

class RawClient
{
public:
    explicit RawClient(std::uint16_t serverPort)
    {
        REQUIRE(enet_initialize() == 0);

        ENetAddress local{};
        REQUIRE(enet_address_set_host_ip(&local, "127.0.0.1") == 0);
        host = enet_host_create(&local, 1, 2, 0, 0);
        REQUIRE(host != nullptr);

        ENetAddress server{};
        REQUIRE(enet_address_set_host_ip(&server, "127.0.0.1") == 0);
        server.port = serverPort;
        peer = enet_host_connect(host, &server, 2, 0);
        REQUIRE(peer != nullptr);
    }

    RawClient(const RawClient&) = delete;
    RawClient& operator=(const RawClient&) = delete;
    RawClient(RawClient&&) = delete;
    RawClient& operator=(RawClient&&) = delete;

    ~RawClient()
    {
        enet_host_destroy(host);
        enet_deinitialize();
    }

    void connectThrough(unison::net::EnetTransport& server)
    {
        Inbox ignored;
        const auto giveUpAt = std::chrono::steady_clock::now() + kPatience;

        while (!isConnected && std::chrono::steady_clock::now() < giveUpAt)
        {
            server.poll(ignored);
            service();
        }

        REQUIRE(isConnected);
    }

    void send(std::uint8_t channel, std::span<const std::byte> bytes, bool isReliable)
    {
        ENetPacket* packet =
            enet_packet_create(bytes.data(), bytes.size(), isReliable ? ENET_PACKET_FLAG_RELIABLE : 0U);
        REQUIRE(enet_peer_send(peer, channel, packet) == 0);
        enet_host_flush(host);
    }

    void service()
    {
        ENetEvent event{};

        while (enet_host_service(host, &event, 1) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_CONNECT)
            {
                isConnected = true;
            }

            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                received.emplace_back(event.packet->data, event.packet->data + event.packet->dataLength);
                enet_packet_destroy(event.packet);
            }
        }
    }

    std::vector<std::vector<std::uint8_t>> received;

private:
    ENetHost* host = nullptr;
    ENetPeer* peer = nullptr;
    bool isConnected = false;
};

std::optional<Received> firstMessageAt(unison::net::EnetTransport& server, RawClient& client)
{
    Inbox inbox;
    const auto giveUpAt = std::chrono::steady_clock::now() + kPatience;

    while (inbox.messages.empty() && std::chrono::steady_clock::now() < giveUpAt)
    {
        client.service();
        server.poll(inbox);
    }

    if (inbox.messages.empty())
    {
        return std::nullopt;
    }

    return inbox.messages.front();
}

const std::array<std::byte, 3> kHello{std::byte{1}, std::byte{2}, std::byte{3}};

}

TEST_CASE("a transport listens on a port the system picks when it is asked for none in particular")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();

    REQUIRE(server->port() != 0U);
}

TEST_CASE("listening on a port another transport holds already fails")
{
    const std::unique_ptr<unison::net::EnetTransport> first = listeningServer();

    const auto second =
        unison::net::EnetTransport::listen(unison::net::EnetAddress{"127.0.0.1", first->port()}, kPeers);

    REQUIRE_FALSE(second.has_value());
    REQUIRE(second.error().code() == unison::ErrorCode::NetworkUnavailable);
}

TEST_CASE("listening on an address that is no address fails")
{
    const auto server = unison::net::EnetTransport::listen(unison::net::EnetAddress{"not an address", 0}, kPeers);

    REQUIRE_FALSE(server.has_value());
    REQUIRE(server.error().code() == unison::ErrorCode::NetworkUnavailable);
}

TEST_CASE("a listening transport receives a client's reliable message on the reliable channel")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    RawClient client{server->port()};
    client.connectThrough(*server);

    client.send(0, kHello, true);
    const std::optional<Received> received = firstMessageAt(*server, client);

    REQUIRE(received.has_value());
    REQUIRE(received->channel == unison::net::Channel::Reliable);
    REQUIRE(received->bytes == std::vector<std::byte>(kHello.begin(), kHello.end()));
}

TEST_CASE("a listening transport receives a client's unreliable message on the unreliable channel")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    RawClient client{server->port()};
    client.connectThrough(*server);

    client.send(1, kHello, false);
    const std::optional<Received> received = firstMessageAt(*server, client);

    REQUIRE(received.has_value());
    REQUIRE(received->channel == unison::net::Channel::Unreliable);
}

TEST_CASE("a listening transport answers a client by the name it gave it")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    RawClient client{server->port()};
    client.connectThrough(*server);
    client.send(0, kHello, true);
    const std::optional<Received> received = firstMessageAt(*server, client);
    REQUIRE(received.has_value());

    server->send(received->from, unison::net::Channel::Reliable, kHello);
    const auto giveUpAt = std::chrono::steady_clock::now() + kPatience;

    while (client.received.empty() && std::chrono::steady_clock::now() < giveUpAt)
    {
        Inbox ignored;
        server->poll(ignored);
        client.service();
    }

    REQUIRE(client.received == std::vector<std::vector<std::uint8_t>>{{1, 2, 3}});
}

namespace
{

class Echo final : public unison::net::IMessageReceiver
{
public:
    explicit Echo(unison::net::ITransport& transport) : transport{transport}
    {
    }

    void receive(unison::net::PeerId from, unison::net::Channel channel, std::span<const std::byte> message) override
    {
        transport.send(from, channel, message);
    }

private:
    unison::net::ITransport& transport;
};

unison::net::EnetConnection connectionTo(const unison::net::EnetTransport& server)
{
    auto connection = unison::net::EnetTransport::connect(unison::net::EnetAddress{"127.0.0.1", server.port()},
                                                          unison::net::EnetAddress{"127.0.0.1", 0});

    REQUIRE(connection.has_value());

    return std::move(*connection);
}

std::vector<Received> messagesUntil(std::size_t count,
                                    unison::net::ITransport& waiting,
                                    unison::net::ITransport& other,
                                    unison::net::IMessageReceiver& otherReceiver)
{
    Inbox inbox;
    const auto giveUpAt = std::chrono::steady_clock::now() + kPatience;

    while (inbox.messages.size() < count && std::chrono::steady_clock::now() < giveUpAt)
    {
        other.poll(otherReceiver);
        waiting.poll(inbox);
    }

    return inbox.messages;
}

}

TEST_CASE("a connecting transport hears its message echoed by a listening one on localhost")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    Echo echo{*server};
    const unison::net::EnetConnection client = connectionTo(*server);

    client.transport->send(client.server, unison::net::Channel::Reliable, kHello);
    const std::vector<Received> echoed = messagesUntil(1, *client.transport, *server, echo);

    REQUIRE(echoed.size() == 1U);
    REQUIRE(echoed.front().from == client.server);
    REQUIRE(echoed.front().channel == unison::net::Channel::Reliable);
    REQUIRE(echoed.front().bytes == std::vector<std::byte>(kHello.begin(), kHello.end()));
}

TEST_CASE("messages sent before the connection is up go out once it is, in the order they were sent")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    const unison::net::EnetConnection client = connectionTo(*server);
    Inbox ignored;

    for (std::uint8_t value = 1; value <= 3; ++value)
    {
        const std::array<std::byte, 1> message{std::byte{value}};
        client.transport->send(client.server, unison::net::Channel::Reliable, message);
    }

    const std::vector<Received> received = messagesUntil(3, *server, *client.transport, ignored);

    REQUIRE(received.size() == 3U);
    REQUIRE(received[0].bytes == std::vector<std::byte>{std::byte{1}});
    REQUIRE(received[1].bytes == std::vector<std::byte>{std::byte{2}});
    REQUIRE(received[2].bytes == std::vector<std::byte>{std::byte{3}});
}

TEST_CASE("connecting to an address that is no address fails")
{
    const auto connection = unison::net::EnetTransport::connect(unison::net::EnetAddress{"not an address", 7000});

    REQUIRE_FALSE(connection.has_value());
    REQUIRE(connection.error().code() == unison::ErrorCode::NetworkUnavailable);
}

TEST_CASE("a transport that goes away says goodbye, so the other side hears of it at once")
{
    REQUIRE(enet_initialize() == 0);
    ENetAddress rawServerAddress{};
    REQUIRE(enet_address_set_host_ip(&rawServerAddress, "127.0.0.1") == 0);
    ENetHost* rawServer = enet_host_create(&rawServerAddress, 1, 2, 0, 0);
    REQUIRE(rawServer != nullptr);
    auto connected = unison::net::EnetTransport::connect(unison::net::EnetAddress{"127.0.0.1", rawServer->address.port},
                                                         unison::net::EnetAddress{"127.0.0.1", 0});
    REQUIRE(connected.has_value());
    std::optional<unison::net::EnetConnection> client{std::move(*connected)};
    Inbox ignored;
    bool isConnected = false;
    bool isGone = false;
    ENetEvent event{};
    const auto giveUpConnectingAt = std::chrono::steady_clock::now() + kPatience;

    while (!isConnected && std::chrono::steady_clock::now() < giveUpConnectingAt)
    {
        client->transport->poll(ignored);
        isConnected = enet_host_service(rawServer, &event, 1) > 0 && event.type == ENET_EVENT_TYPE_CONNECT;
    }

    client.reset();
    const auto giveUpWaitingAt = std::chrono::steady_clock::now() + std::chrono::milliseconds{500};

    while (!isGone && std::chrono::steady_clock::now() < giveUpWaitingAt)
    {
        isGone = enet_host_service(rawServer, &event, 1) > 0 && event.type == ENET_EVENT_TYPE_DISCONNECT;
    }

    enet_host_destroy(rawServer);
    enet_deinitialize();

    REQUIRE(isConnected);
    REQUIRE(isGone);
}

namespace
{

std::vector<unison::net::PeerId> departuresWithin(std::chrono::milliseconds patience, unison::net::ITransport& watching)
{
    Inbox inbox;
    const auto giveUpAt = std::chrono::steady_clock::now() + patience;

    while (inbox.departures.empty() && std::chrono::steady_clock::now() < giveUpAt)
    {
        watching.poll(inbox);
    }

    return inbox.departures;
}

}

TEST_CASE("a peer that says goodbye is reported gone at once")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    Echo echo{*server};
    std::optional<unison::net::EnetConnection> client{connectionTo(*server)};
    client->transport->send(client->server, unison::net::Channel::Reliable, kHello);
    const std::vector<Received> echoed = messagesUntil(1, *client->transport, *server, echo);
    REQUIRE(echoed.size() == 1U);

    client.reset();
    const std::vector<unison::net::PeerId> departures = departuresWithin(std::chrono::milliseconds{500}, *server);

    REQUIRE(departures.size() == 1U);
}

TEST_CASE("a peer that stops answering is reported gone once its timeout has passed")
{
    auto listening = unison::net::EnetTransport::listen(
        unison::net::EnetAddress{"127.0.0.1", 0}, kPeers, std::chrono::milliseconds{250});
    REQUIRE(listening.has_value());
    const std::unique_ptr<unison::net::EnetTransport> server = std::move(*listening);
    RawClient silentClient{server->port()};
    silentClient.connectThrough(*server);

    const std::vector<unison::net::PeerId> departures = departuresWithin(std::chrono::seconds{3}, *server);

    REQUIRE(departures.size() == 1U);
}

TEST_CASE("a client whose server goes away hears of it")
{
    std::optional<std::unique_ptr<unison::net::EnetTransport>> server{listeningServer()};
    Echo echo{**server};
    const unison::net::EnetConnection client = connectionTo(**server);
    client.transport->send(client.server, unison::net::Channel::Reliable, kHello);
    const std::vector<Received> echoed = messagesUntil(1, *client.transport, **server, echo);
    REQUIRE(echoed.size() == 1U);

    server.reset();
    const std::vector<unison::net::PeerId> departures =
        departuresWithin(std::chrono::milliseconds{500}, *client.transport);

    REQUIRE(departures == std::vector<unison::net::PeerId>{client.server});
}

TEST_CASE("an unreliable message longer than a datagram breaks a contract and is not sent")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    const unison::net::EnetConnection client = connectionTo(*server);
    Inbox ignored;
    client.transport->send(client.server, unison::net::Channel::Reliable, kHello);
    const std::vector<Received> connected = messagesUntil(1, *server, *client.transport, ignored);
    const std::vector<std::byte> tooLong(unison::net::kMaxUnreliableMessageSize + 1U);
    const std::array<std::byte, 1> marker{std::byte{9}};
    const unison::test::FatalHandlerProbe probe;

    client.transport->send(client.server, unison::net::Channel::Unreliable, tooLong);
    client.transport->send(client.server, unison::net::Channel::Reliable, marker);
    const std::vector<Received> received = messagesUntil(1, *server, *client.transport, ignored);

    REQUIRE(connected.size() == 1U);
    REQUIRE(probe.failureCount() == 1U);
    REQUIRE(received.size() == 1U);
    REQUIRE(received.front().bytes == std::vector<std::byte>{std::byte{9}});
}

TEST_CASE("a reliable message far longer than a datagram arrives whole")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    const unison::net::EnetConnection client = connectionTo(*server);
    Inbox ignored;
    std::vector<std::byte> hundredDatagrams(unison::net::kMaxUnreliableMessageSize * 100U);

    for (std::size_t index = 0; index < hundredDatagrams.size(); ++index)
    {
        hundredDatagrams[index] = static_cast<std::byte>(index % 251U);
    }

    client.transport->send(client.server, unison::net::Channel::Reliable, hundredDatagrams);
    const std::vector<Received> received = messagesUntil(1, *server, *client.transport, ignored);

    REQUIRE(received.size() == 1U);
    REQUIRE(received.front().bytes == hundredDatagrams);
}

TEST_CASE("two clients of a listening transport go by different names")
{
    const std::unique_ptr<unison::net::EnetTransport> server = listeningServer();
    RawClient first{server->port()};
    RawClient second{server->port()};
    first.connectThrough(*server);
    second.connectThrough(*server);

    first.send(0, kHello, true);
    const std::optional<Received> fromFirst = firstMessageAt(*server, first);
    second.send(0, kHello, true);
    const std::optional<Received> fromSecond = firstMessageAt(*server, second);

    REQUIRE(fromFirst.has_value());
    REQUIRE(fromSecond.has_value());
    REQUIRE(fromFirst->from != fromSecond->from);
}
