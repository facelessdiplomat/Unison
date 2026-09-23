#include <unison/net/outbox.hpp>

#include <unison/net/loopback_hub.hpp>
#include <unison/net/message_codec.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <variant>
#include <vector>

namespace
{

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
        received.push_back(Received{from, channel, {message.begin(), message.end()}});
    }

    std::vector<Received> received;
};

}

TEST_CASE("an outbox hands the transport a message as the wire carries it")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& receiver = hub.join();
    unison::net::Outbox outbox{sender};
    Inbox inbox;

    outbox.send(receiver.id(), unison::net::Channel::Unreliable, unison::net::Ping{123});
    receiver.poll(inbox);

    REQUIRE(inbox.received.size() == 1U);
    REQUIRE(inbox.received.front().from == sender.id());
    REQUIRE(inbox.received.front().channel == unison::net::Channel::Unreliable);
    const auto decoded = unison::net::decode(inbox.received.front().bytes);
    REQUIRE(decoded.has_value());
    REQUIRE(std::get<unison::net::Ping>(*decoded).sentAt == 123U);
}

TEST_CASE("a message larger than a datagram breaks a contract and is not sent")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& receiver = hub.join();
    unison::net::Outbox outbox{sender};
    std::array<std::byte, unison::net::kMaxDatagramSize> inputs{};
    Inbox inbox;
    const unison::test::FatalHandlerProbe probe;

    outbox.send(receiver.id(),
                unison::net::Channel::Reliable,
                unison::net::Input{1, 60, static_cast<std::uint8_t>(inputs.size() / 60U), inputs});
    receiver.poll(inbox);

    REQUIRE(probe.failureCount() == 1U);
    REQUIRE(inbox.received.empty());
}
