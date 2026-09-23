#include <unison/net/enet_transport.hpp>

#include <catch2/catch_test_macros.hpp>

#include <enet/enet.h>

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

    std::vector<Received> messages;
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
