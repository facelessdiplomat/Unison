#include <unison/net/relay_core.hpp>

#include <unison/net/loopback_hub.hpp>
#include <unison/net/message_codec.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

namespace
{

using Replies = std::vector<std::vector<std::byte>>;

class ReplyCollector final : public unison::net::IMessageReceiver
{
public:
    explicit ReplyCollector(Replies& replies) : replies{replies}
    {
    }

    void receive(unison::net::PeerId, unison::net::Channel, std::span<const std::byte> message) override
    {
        replies.emplace_back(message.begin(), message.end());
    }

private:
    Replies& replies;
};

template <typename T>
T onlyReplyAs(const Replies& replies)
{
    REQUIRE(replies.size() == 1U);

    const auto decoded = unison::net::decode(replies.front());

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<T>(*decoded));

    return std::get<T>(*decoded);
}

unison::net::SessionConfig twoPlayers()
{
    unison::net::SessionConfig config;
    config.slotCount = 2;
    config.inputSize = 8;
    config.seed = 7;

    return config;
}

struct Relay
{
    explicit Relay(const unison::net::SessionConfig& config) : endpoint{hub.join()}, core{endpoint, config}
    {
    }

    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& endpoint;
    unison::net::RelayCore core;
};

void sendMessage(unison::net::ITransport& from, unison::net::PeerId to, const unison::net::Message& message)
{
    std::array<std::byte, 256> buffer{};
    const auto written = unison::net::encode(message, buffer);

    REQUIRE(written.has_value());
    from.send(to, unison::net::Channel::Reliable, std::span{buffer}.first(*written));
}

Replies repliesTo(Relay& relay, const unison::net::Hello& hello)
{
    unison::net::LoopbackEndpoint& client = relay.hub.join();
    sendMessage(client, relay.endpoint.id(), hello);
    relay.endpoint.poll(relay.core);

    Replies replies;
    ReplyCollector collector{replies};
    client.poll(collector);

    return replies;
}

unison::net::Hello helloFor(const unison::net::SessionConfig& config, unison::net::Role role)
{
    return unison::net::Hello{unison::net::kProtocolVersion, unison::net::hashOf(config), role, 0};
}

}

TEST_CASE("the first two players to say hello are welcomed into slots zero and one")
{
    Relay relay{twoPlayers()};

    const Replies first = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player));
    const Replies second = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player));

    REQUIRE(onlyReplyAs<unison::net::Welcome>(first).slot == 0U);
    REQUIRE(onlyReplyAs<unison::net::Welcome>(second).slot == 1U);
}

TEST_CASE("a welcome carries the config everyone in the room plays")
{
    Relay relay{twoPlayers()};

    const Replies replies = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player));

    REQUIRE(unison::net::hashOf(onlyReplyAs<unison::net::Welcome>(replies).config) ==
            unison::net::hashOf(twoPlayers()));
}

TEST_CASE("a client that would play another config is kicked")
{
    Relay relay{twoPlayers()};
    unison::net::SessionConfig other = twoPlayers();
    other.seed = 8;

    const Replies replies = repliesTo(relay, helloFor(other, unison::net::Role::Player));

    REQUIRE(onlyReplyAs<unison::net::Kick>(replies).reason == unison::net::LeaveReason::ConfigMismatch);
}

TEST_CASE("a client that speaks another protocol version is kicked")
{
    Relay relay{twoPlayers()};
    unison::net::Hello hello = helloFor(twoPlayers(), unison::net::Role::Player);
    hello.protocolVersion = unison::net::kProtocolVersion + 1;

    const Replies replies = repliesTo(relay, hello);

    REQUIRE(onlyReplyAs<unison::net::Kick>(replies).reason == unison::net::LeaveReason::ProtocolMismatch);
}

TEST_CASE("a player who finds every slot taken is kicked")
{
    Relay relay{twoPlayers()};
    static_cast<void>(repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player)));
    static_cast<void>(repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player)));

    const Replies third = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player));

    REQUIRE(onlyReplyAs<unison::net::Kick>(third).reason == unison::net::LeaveReason::RoomFull);
}

TEST_CASE("a spectator is welcomed without taking a slot")
{
    Relay relay{twoPlayers()};

    const Replies spectator = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Spectator));
    const Replies player = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player));

    REQUIRE(onlyReplyAs<unison::net::Welcome>(spectator).slot == unison::net::kNoSlot);
    REQUIRE(onlyReplyAs<unison::net::Welcome>(player).slot == 0U);
}

TEST_CASE("bytes that are no message go unanswered")
{
    Relay relay{twoPlayers()};
    unison::net::LoopbackEndpoint& client = relay.hub.join();
    const std::array<std::byte, 3> garbage{std::byte{0xEE}, std::byte{1}, std::byte{2}};
    client.send(relay.endpoint.id(), unison::net::Channel::Reliable, garbage);

    relay.endpoint.poll(relay.core);

    Replies replies;
    ReplyCollector collector{replies};
    client.poll(collector);
    REQUIRE(replies.empty());
}
