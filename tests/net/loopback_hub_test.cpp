#include <unison/net/loopback_hub.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace
{

struct Received
{
    unison::net::PeerId from{};
    unison::net::Channel channel = unison::net::Channel::Reliable;
    std::vector<std::byte> bytes;
};

class RecordingReceiver final : public unison::net::IMessageReceiver
{
public:
    void receive(unison::net::PeerId from, unison::net::Channel channel, std::span<const std::byte> message) override
    {
        received.push_back(Received{from, channel, {message.begin(), message.end()}});
    }

    [[nodiscard]] const std::vector<Received>& messages() const
    {
        return received;
    }

private:
    std::vector<Received> received;
};

class EchoingReceiver final : public unison::net::IMessageReceiver
{
public:
    explicit EchoingReceiver(unison::net::ITransport& transport) : transport{transport}
    {
    }

    void receive(unison::net::PeerId from, unison::net::Channel channel, std::span<const std::byte> message) override
    {
        transport.send(from, channel, message);
        ++echoed;
    }

    [[nodiscard]] std::uint32_t echoCount() const
    {
        return echoed;
    }

private:
    unison::net::ITransport& transport;
    std::uint32_t echoed = 0;
};

std::array<std::byte, 1> messageOf(std::uint8_t value)
{
    return {std::byte{value}};
}

}

TEST_CASE("endpoints are numbered in the order they joined the hub")
{
    unison::net::LoopbackHub hub;

    const unison::net::LoopbackEndpoint& first = hub.join();
    const unison::net::LoopbackEndpoint& second = hub.join();

    REQUIRE(first.id() == unison::net::PeerId{0});
    REQUIRE(second.id() == unison::net::PeerId{1});
}

TEST_CASE("messages from one endpoint reach another in the order they were sent")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& listener = hub.join();
    RecordingReceiver receiver;

    for (std::uint8_t value = 1; value <= 3; ++value)
    {
        sender.send(listener.id(), unison::net::Channel::Unreliable, messageOf(value));
    }

    listener.poll(receiver);

    REQUIRE(receiver.messages().size() == 3U);
    REQUIRE(receiver.messages()[0].bytes == std::vector<std::byte>{std::byte{1}});
    REQUIRE(receiver.messages()[1].bytes == std::vector<std::byte>{std::byte{2}});
    REQUIRE(receiver.messages()[2].bytes == std::vector<std::byte>{std::byte{3}});
}

TEST_CASE("a received message names the endpoint it came from and its channel")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& listener = hub.join();
    RecordingReceiver receiver;
    sender.send(listener.id(), unison::net::Channel::Reliable, messageOf(7));

    listener.poll(receiver);

    REQUIRE(receiver.messages().size() == 1U);
    REQUIRE(receiver.messages()[0].from == sender.id());
    REQUIRE(receiver.messages()[0].channel == unison::net::Channel::Reliable);
}

TEST_CASE("a poll hands each message over once")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& listener = hub.join();
    RecordingReceiver receiver;
    sender.send(listener.id(), unison::net::Channel::Reliable, messageOf(7));

    listener.poll(receiver);
    listener.poll(receiver);

    REQUIRE(receiver.messages().size() == 1U);
}

TEST_CASE("an endpoint hears only the messages meant for it")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& addressed = hub.join();
    unison::net::LoopbackEndpoint& bystander = hub.join();
    RecordingReceiver addressedReceiver;
    RecordingReceiver bystanderReceiver;
    sender.send(addressed.id(), unison::net::Channel::Reliable, messageOf(7));

    addressed.poll(addressedReceiver);
    bystander.poll(bystanderReceiver);

    REQUIRE(addressedReceiver.messages().size() == 1U);
    REQUIRE(bystanderReceiver.messages().empty());
}

TEST_CASE("a message sent back while polling waits for the next poll")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& first = hub.join();
    unison::net::LoopbackEndpoint& second = hub.join();
    EchoingReceiver echo{second};
    RecordingReceiver receiver;
    first.send(second.id(), unison::net::Channel::Reliable, messageOf(7));
    second.poll(echo);

    first.poll(receiver);

    REQUIRE(echo.echoCount() == 1U);
    REQUIRE(receiver.messages().size() == 1U);
    REQUIRE(receiver.messages()[0].from == second.id());
}

TEST_CASE("sending to a peer the hub does not know breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();

    sender.send(unison::net::PeerId{5}, unison::net::Channel::Reliable, messageOf(7));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("an unreliable message longer than a datagram breaks a contract and is not carried")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& receiver = hub.join();
    const std::vector<std::byte> tooLong(unison::net::kMaxUnreliableMessageSize + 1U);
    const unison::test::FatalHandlerProbe probe;

    sender.send(receiver.id(), unison::net::Channel::Unreliable, tooLong);

    RecordingReceiver recording;
    receiver.poll(recording);
    REQUIRE(probe.failureCount() == 1U);
    REQUIRE(recording.messages().empty());
}

TEST_CASE("a reliable message longer than a datagram is carried whole")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& receiver = hub.join();
    const std::vector<std::byte> tenDatagrams(unison::net::kMaxUnreliableMessageSize * 10U, std::byte{7});

    sender.send(receiver.id(), unison::net::Channel::Reliable, tenDatagrams);

    RecordingReceiver recording;
    receiver.poll(recording);
    REQUIRE(recording.messages().size() == 1U);
    REQUIRE(recording.messages().front().bytes == tenDatagrams);
}
