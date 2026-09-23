#include <unison/net/relay_core.hpp>

#include <unison/net/loopback_hub.hpp>
#include <unison/net/message_codec.hpp>
#include <unison/net/network_simulator.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <algorithm>
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
    explicit Relay(const unison::net::SessionConfig& config) : endpoint{hub.join()}, core{endpoint, clock, config}
    {
    }

    unison::net::LoopbackHub hub;
    unison::net::ManualClock clock;
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

namespace
{

unison::net::SessionConfig threeSlotsOfTwoBytes()
{
    unison::net::SessionConfig config;
    config.slotCount = 3;
    config.inputSize = 2;

    return config;
}

struct Match
{
    Match() : relay{threeSlotsOfTwoBytes()}, first{relay.hub.join()}, second{relay.hub.join()}
    {
        sendMessage(first, relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
        sendMessage(second, relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
        relay.endpoint.poll(relay.core);
        static_cast<void>(repliesOf(first));
        static_cast<void>(repliesOf(second));
    }

    static Replies repliesOf(unison::net::ITransport& client)
    {
        Replies replies;
        ReplyCollector collector{replies};
        client.poll(collector);

        return replies;
    }

    void sendInputs(unison::net::ITransport& client, std::uint32_t firstFrame, std::span<const std::byte> inputs)
    {
        const auto frameCount = static_cast<std::uint8_t>(inputs.size() / 2);
        sendMessage(client, relay.endpoint.id(), unison::net::Input{firstFrame, 2, frameCount, inputs});
        relay.endpoint.poll(relay.core);
    }

    Relay relay;
    unison::net::LoopbackEndpoint& first;
    unison::net::LoopbackEndpoint& second;
};

std::array<std::byte, 2> inputOf(std::uint8_t value)
{
    return {std::byte{value}, std::byte{value}};
}

std::vector<unison::net::Confirmed> confirmationsIn(const Replies& replies)
{
    std::vector<unison::net::Confirmed> confirmations;

    for (const std::vector<std::byte>& reply : replies)
    {
        const auto decoded = unison::net::decode(reply);

        REQUIRE(decoded.has_value());

        if (const auto* confirmed = std::get_if<unison::net::Confirmed>(&*decoded))
        {
            confirmations.push_back(*confirmed);
        }
    }

    return confirmations;
}

std::size_t frameSizeOf(const unison::net::Confirmed& confirmed)
{
    return unison::net::confirmedFrameSize(confirmed.slotCount, confirmed.inputSize);
}

std::uint32_t newestFrameOf(const unison::net::Confirmed& confirmed)
{
    return confirmed.firstFrame + confirmed.frameCount - 1U;
}

std::span<const std::byte> slotsOfFrame(const unison::net::Confirmed& confirmed, std::uint32_t frame)
{
    REQUIRE(frame >= confirmed.firstFrame);
    REQUIRE(frame <= newestFrameOf(confirmed));

    return confirmed.slots.subspan((frame - confirmed.firstFrame) * frameSizeOf(confirmed), frameSizeOf(confirmed));
}

std::vector<std::uint32_t> framesIn(const std::vector<unison::net::Confirmed>& confirmations)
{
    std::vector<std::uint32_t> frames;

    for (const unison::net::Confirmed& confirmed : confirmations)
    {
        for (std::uint32_t frame = confirmed.firstFrame; frame <= newestFrameOf(confirmed); ++frame)
        {
            frames.push_back(frame);
        }
    }

    return frames;
}

}

TEST_CASE("a frame is confirmed with the input of every player once all of them have sent one")
{
    Match match;
    match.sendInputs(match.first, 1, inputOf(10));
    match.sendInputs(match.second, 1, inputOf(20));

    const Replies replies = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);

    REQUIRE(confirmations.size() == 1U);
    REQUIRE(confirmations[0].firstFrame == 1U);
    REQUIRE(confirmations[0].frameCount == 1U);
    REQUIRE(confirmations[0].slotCount == 3U);
    REQUIRE(confirmations[0].inputSize == 2U);

    const std::array<std::byte, 9> slots{std::byte{1},
                                         std::byte{10},
                                         std::byte{10},
                                         std::byte{1},
                                         std::byte{20},
                                         std::byte{20},
                                         std::byte{0},
                                         std::byte{0},
                                         std::byte{0}};
    REQUIRE(std::ranges::equal(confirmations[0].slots, slots));
}

TEST_CASE("a frame is not confirmed while a player's input is missing")
{
    Match match;

    match.sendInputs(match.first, 1, inputOf(10));

    REQUIRE(confirmationsIn(Match::repliesOf(match.first)).empty());
}

TEST_CASE("frames are confirmed in order, and each of them once")
{
    Match match;
    const std::array<std::byte, 6> threeFrames{
        std::byte{1}, std::byte{1}, std::byte{2}, std::byte{2}, std::byte{3}, std::byte{3}};
    match.sendInputs(match.second, 1, threeFrames);
    match.sendInputs(match.first, 1, threeFrames);
    match.sendInputs(match.first, 1, threeFrames);

    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(Match::repliesOf(match.second));

    REQUIRE(confirmations.size() == 3U);
    REQUIRE(newestFrameOf(confirmations[0]) == 1U);
    REQUIRE(newestFrameOf(confirmations[1]) == 2U);
    REQUIRE(newestFrameOf(confirmations[2]) == 3U);
}

TEST_CASE("a confirmation also carries the three frames confirmed before it")
{
    Match match;

    for (std::uint8_t frame = 1; frame <= 5; ++frame)
    {
        match.sendInputs(match.first, frame, inputOf(frame));
        match.sendInputs(match.second, frame, inputOf(static_cast<std::uint8_t>(100U + frame)));
    }

    const Replies replies = Match::repliesOf(match.first);
    const unison::net::Confirmed newest = confirmationsIn(replies).back();

    REQUIRE(newest.firstFrame == 2U);
    REQUIRE(newest.frameCount == 4U);

    for (std::uint32_t frame = 2; frame <= 5; ++frame)
    {
        CAPTURE(frame);

        REQUIRE(slotsOfFrame(newest, frame)[1] == std::byte{static_cast<std::uint8_t>(frame)});
    }
}

TEST_CASE("the first confirmations carry only the frames confirmed so far")
{
    Match match;

    for (std::uint8_t frame = 1; frame <= 2; ++frame)
    {
        match.sendInputs(match.first, frame, inputOf(frame));
        match.sendInputs(match.second, frame, inputOf(frame));
    }

    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(Match::repliesOf(match.first));

    REQUIRE(confirmations.size() == 2U);
    REQUIRE(confirmations[0].firstFrame == 1U);
    REQUIRE(confirmations[0].frameCount == 1U);
    REQUIRE(confirmations[1].firstFrame == 1U);
    REQUIRE(confirmations[1].frameCount == 2U);
}

TEST_CASE("a confirmation carries no more frames than fit in one datagram")
{
    unison::net::SessionConfig config;
    config.slotCount = unison::net::kMaxSlots;
    config.inputSize = 64;
    Relay relay{config};
    unison::net::LoopbackEndpoint& player = relay.hub.join();
    sendMessage(player, relay.endpoint.id(), helloFor(config, unison::net::Role::Player));
    const std::array<std::byte, 64> input{};

    for (std::uint32_t frame = 1; frame <= 3; ++frame)
    {
        sendMessage(player, relay.endpoint.id(), unison::net::Input{frame, 64, 1, input});
    }

    relay.endpoint.poll(relay.core);

    const Replies replies = Match::repliesOf(player);
    const unison::net::Confirmed newest = confirmationsIn(replies).back();

    REQUIRE(newestFrameOf(newest) == 3U);
    REQUIRE(newest.frameCount == unison::net::confirmedFramesPerDatagram(config.slotCount, config.inputSize));
}

TEST_CASE("a spectator is sent every confirmed frame too")
{
    Match match;
    unison::net::LoopbackEndpoint& spectator = match.relay.hub.join();
    sendMessage(spectator, match.relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Spectator));
    match.relay.endpoint.poll(match.relay.core);
    static_cast<void>(Match::repliesOf(spectator));

    match.sendInputs(match.first, 1, inputOf(10));
    match.sendInputs(match.second, 1, inputOf(20));

    REQUIRE(confirmationsIn(Match::repliesOf(spectator)).size() == 1U);
}

TEST_CASE("inputs from a client without a slot are ignored")
{
    Match match;
    unison::net::LoopbackEndpoint& spectator = match.relay.hub.join();
    sendMessage(spectator, match.relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Spectator));
    match.relay.endpoint.poll(match.relay.core);

    match.sendInputs(spectator, 1, inputOf(30));
    match.sendInputs(match.first, 1, inputOf(10));

    REQUIRE(confirmationsIn(Match::repliesOf(match.first)).empty());
}

TEST_CASE("inputs of another size than the config's are ignored")
{
    Match match;
    const std::array<std::byte, 3> threeBytes{std::byte{1}, std::byte{2}, std::byte{3}};
    sendMessage(match.first, match.relay.endpoint.id(), unison::net::Input{1, 3, 1, threeBytes});
    match.relay.endpoint.poll(match.relay.core);

    match.sendInputs(match.second, 1, inputOf(20));

    REQUIRE(confirmationsIn(Match::repliesOf(match.first)).empty());
}

TEST_CASE("a frame is not confirmed before its deadline while a player's input is missing")
{
    Match match;
    match.sendInputs(match.first, 1, inputOf(10));

    match.relay.clock.advance(99'999);
    match.relay.core.update();

    REQUIRE(confirmationsIn(Match::repliesOf(match.first)).empty());
}

TEST_CASE("a frame is confirmed at its deadline with the missing player's input dropped")
{
    Match match;
    match.sendInputs(match.first, 1, inputOf(10));

    match.relay.clock.advance(100'000);
    match.relay.core.update();

    const Replies replies = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);
    const std::array<std::byte, 9> slots{std::byte{1},
                                         std::byte{10},
                                         std::byte{10},
                                         std::byte{2},
                                         std::byte{0},
                                         std::byte{0},
                                         std::byte{0},
                                         std::byte{0},
                                         std::byte{0}};
    REQUIRE(confirmations.size() == 1U);
    REQUIRE(std::ranges::equal(confirmations[0].slots, slots));
}

TEST_CASE("a player dropped from a frame repeats the last input confirmed for it")
{
    Match match;
    match.sendInputs(match.first, 1, inputOf(10));
    match.sendInputs(match.second, 1, inputOf(20));
    match.sendInputs(match.first, 2, inputOf(11));
    static_cast<void>(Match::repliesOf(match.first));

    match.relay.clock.advance(100'000);
    match.relay.core.update();

    const Replies replies = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);
    const std::array<std::byte, 3> repeated{std::byte{2}, std::byte{20}, std::byte{20}};
    REQUIRE(confirmations.size() == 1U);
    REQUIRE(newestFrameOf(confirmations[0]) == 2U);
    REQUIRE(std::ranges::equal(slotsOfFrame(confirmations[0], 2).subspan(3, 3), repeated));
}

TEST_CASE("an input that arrives after its frame was confirmed without it is ignored")
{
    Match match;
    match.sendInputs(match.first, 1, inputOf(10));
    match.relay.clock.advance(100'000);
    match.relay.core.update();
    static_cast<void>(Match::repliesOf(match.first));

    match.sendInputs(match.second, 1, inputOf(20));

    REQUIRE(confirmationsIn(Match::repliesOf(match.first)).empty());
}

TEST_CASE("an input message lost on the way costs nothing, since the next one repeats its inputs")
{
    constexpr std::uint32_t kFrames = 6;
    constexpr std::uint32_t kRedundancy = 4;
    constexpr std::uint32_t kLostFrame = 3;
    Match match;

    for (std::uint32_t frame = 1; frame <= kFrames; ++frame)
    {
        const std::uint32_t oldest = frame > kRedundancy - 1 ? frame - (kRedundancy - 1) : 1;
        std::vector<std::byte> recent;

        for (std::uint32_t repeated = oldest; repeated <= frame; ++repeated)
        {
            const std::array<std::byte, 2> input = inputOf(static_cast<std::uint8_t>(repeated));
            recent.insert(recent.end(), input.begin(), input.end());
        }

        if (frame != kLostFrame)
        {
            match.sendInputs(match.first, oldest, recent);
        }

        match.sendInputs(match.second, frame, inputOf(static_cast<std::uint8_t>(100 + frame)));
    }

    const Replies replies = Match::repliesOf(match.second);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);

    REQUIRE(confirmations.size() == kFrames);

    for (std::uint32_t index = 0; index < kFrames; ++index)
    {
        CAPTURE(index);

        const std::span<const std::byte> slots = slotsOfFrame(confirmations[index], index + 1);

        REQUIRE(newestFrameOf(confirmations[index]) == index + 1);
        REQUIRE(slots[0] == std::byte{1});
        REQUIRE(slots[1] == std::byte{static_cast<std::uint8_t>(index + 1)});
    }
}

namespace
{

std::vector<unison::net::Desync> desyncsIn(const Replies& replies)
{
    std::vector<unison::net::Desync> desyncs;

    for (const std::vector<std::byte>& reply : replies)
    {
        const auto decoded = unison::net::decode(reply);

        REQUIRE(decoded.has_value());

        if (const auto* desync = std::get_if<unison::net::Desync>(&*decoded))
        {
            desyncs.push_back(*desync);
        }
    }

    return desyncs;
}

}

TEST_CASE("a player whose checksum parts ways with the others is reported to everyone")
{
    Match match;
    sendMessage(match.first, match.relay.endpoint.id(), unison::net::Checksum{20, 77});
    sendMessage(match.second, match.relay.endpoint.id(), unison::net::Checksum{20, 78});

    match.relay.endpoint.poll(match.relay.core);

    const std::vector<unison::net::Desync> desyncs = desyncsIn(Match::repliesOf(match.first));
    REQUIRE(desyncs.size() == 1U);
    REQUIRE(desyncs[0].frame == 20U);
    REQUIRE(desyncs[0].minoritySlots == 0b11U);
}

TEST_CASE("players whose checksums agree hear nothing about it")
{
    Match match;
    sendMessage(match.first, match.relay.endpoint.id(), unison::net::Checksum{20, 77});
    sendMessage(match.second, match.relay.endpoint.id(), unison::net::Checksum{20, 77});

    match.relay.endpoint.poll(match.relay.core);

    REQUIRE(desyncsIn(Match::repliesOf(match.first)).empty());
}

namespace
{

Replies repliesAcrossALinkLosingEveryUnreliableMessage(std::uint32_t frames)
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& relayEndpoint = hub.join();
    unison::net::NetworkSimulator network{unison::net::NetworkConditions{0, 0, 1.0F}, 20260923};
    unison::net::SimulatedLink lossyRelay{relayEndpoint, network};
    unison::net::ManualClock clock;
    unison::net::RelayCore relay{lossyRelay, clock, threeSlotsOfTwoBytes()};
    unison::net::LoopbackEndpoint& first = hub.join();
    unison::net::LoopbackEndpoint& second = hub.join();
    sendMessage(first, relayEndpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    sendMessage(second, relayEndpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    relayEndpoint.poll(relay);

    for (std::uint32_t frame = 1; frame <= frames; ++frame)
    {
        const std::array<std::byte, 2> input = inputOf(static_cast<std::uint8_t>(frame));
        sendMessage(first, relayEndpoint.id(), unison::net::Input{frame, 2, 1, input});
        sendMessage(second, relayEndpoint.id(), unison::net::Input{frame, 2, 1, input});
        relayEndpoint.poll(relay);
        network.advance(0);
    }

    return Match::repliesOf(first);
}

}

TEST_CASE("a client that loses every unreliable message still receives every confirmed frame")
{
    constexpr std::uint32_t kFrames = 20;

    const Replies replies = repliesAcrossALinkLosingEveryUnreliableMessage(kFrames);
    const std::vector<std::uint32_t> frames = framesIn(confirmationsIn(replies));

    REQUIRE(frames.size() == kFrames);

    for (std::uint32_t index = 0; index < kFrames; ++index)
    {
        REQUIRE(frames[index] == index + 1);
    }
}

TEST_CASE("the frames the relay sends again reliably go out in as few messages as they fit")
{
    const Replies replies = repliesAcrossALinkLosingEveryUnreliableMessage(20);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);

    REQUIRE(confirmations.size() == 2U);
    REQUIRE(confirmations[0].frameCount == 10U);
    REQUIRE(confirmations[1].frameCount == 10U);
}

TEST_CASE("a relay that would never send its confirmed frames again breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& endpoint = hub.join();
    const unison::net::ManualClock clock;
    unison::net::RelaySettings never;
    never.reliableResendInterval = 0;

    const unison::net::RelayCore relay{endpoint, clock, twoPlayers(), never};

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a ping is answered with its own stamp and the frame the relay has confirmed so far")
{
    Match match;
    match.sendInputs(match.first, 1, inputOf(10));
    match.sendInputs(match.second, 1, inputOf(20));
    static_cast<void>(Match::repliesOf(match.first));

    sendMessage(match.first, match.relay.endpoint.id(), unison::net::Ping{123'456});
    match.relay.endpoint.poll(match.relay.core);

    const unison::net::Pong pong = onlyReplyAs<unison::net::Pong>(Match::repliesOf(match.first));
    REQUIRE(pong.pingSentAt == 123'456U);
    REQUIRE(pong.confirmedFrame == 1U);
}

TEST_CASE("a ping and its pong measure the round trip to the relay")
{
    constexpr std::uint32_t kOneWayMilliseconds = 30;
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& relayEndpoint = hub.join();
    unison::net::LoopbackEndpoint& clientEndpoint = hub.join();
    unison::net::NetworkSimulator network{unison::net::NetworkConditions{kOneWayMilliseconds, 0, 0.0F}, 20260923};
    unison::net::SimulatedLink relayLink{relayEndpoint, network};
    unison::net::SimulatedLink clientLink{clientEndpoint, network};
    unison::net::ManualClock clock;
    unison::net::RelayCore relay{relayLink, clock, threeSlotsOfTwoBytes()};
    sendMessage(clientLink, relayEndpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    network.advance(kOneWayMilliseconds);
    relayEndpoint.poll(relay);
    network.advance(kOneWayMilliseconds);
    static_cast<void>(Match::repliesOf(clientEndpoint));
    const std::uint64_t sentAt = 1'000'000;

    sendMessage(clientLink, relayEndpoint.id(), unison::net::Ping{sentAt});
    network.advance(kOneWayMilliseconds);
    relayEndpoint.poll(relay);
    network.advance(kOneWayMilliseconds);
    const std::uint64_t receivedAt = sentAt + 2U * kOneWayMilliseconds * 1'000U;

    const unison::net::Pong pong = onlyReplyAs<unison::net::Pong>(Match::repliesOf(clientEndpoint));
    REQUIRE(receivedAt - pong.pingSentAt == 60'000U);
}

TEST_CASE("a ping from a peer that is not in the match goes unanswered")
{
    Relay relay{twoPlayers()};
    unison::net::LoopbackEndpoint& stranger = relay.hub.join();

    sendMessage(stranger, relay.endpoint.id(), unison::net::Ping{1});
    relay.endpoint.poll(relay.core);

    REQUIRE(Match::repliesOf(stranger).empty());
}

TEST_CASE("a pong carries the newest frame any player has sent an input for")
{
    Match match;
    const std::array<std::byte, 10> fiveFrames{};
    match.sendInputs(match.first, 1, fiveFrames);
    static_cast<void>(Match::repliesOf(match.first));

    sendMessage(match.first, match.relay.endpoint.id(), unison::net::Ping{7});
    match.relay.endpoint.poll(match.relay.core);

    const unison::net::Pong pong = onlyReplyAs<unison::net::Pong>(Match::repliesOf(match.first));
    REQUIRE(pong.confirmedFrame == 0U);
    REQUIRE(pong.newestInputFrame == 5U);
}
