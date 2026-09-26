#include <unison/net/relay_core.hpp>

#include <unison/net/loopback_hub.hpp>
#include <unison/net/message_codec.hpp>
#include <unison/net/network_simulator.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <support/fatal_handler_probe.hpp>
#include <support/fixed_round_trips.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
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

unison::net::Welcome welcomeIn(const Replies& replies)
{
    const auto welcome =
        std::ranges::find_if(replies,
                             [](const std::vector<std::byte>& reply)
                             {
                                 const auto decoded = unison::net::decode(reply);

                                 return decoded.has_value() && std::holds_alternative<unison::net::Welcome>(*decoded);
                             });

    REQUIRE(welcome != replies.end());

    return std::get<unison::net::Welcome>(*unison::net::decode(*welcome));
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
    explicit Relay(const unison::net::SessionConfig& config, const unison::net::IRoundTripMeter* roundTrips = nullptr)
        : endpoint{hub.join()}, core{endpoint, clock, config, unison::net::RelaySettings{}, roundTrips}
    {
    }

    unison::net::LoopbackHub hub;
    unison::net::ManualClock clock;
    unison::net::LoopbackEndpoint& endpoint;
    unison::net::RelayCore core;
};

void sendMessage(unison::net::ITransport& from, unison::net::PeerId to, const unison::net::Message& message)
{
    std::array<std::byte, unison::net::kMaxDatagramSize> buffer{};
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
    return unison::net::Hello{unison::net::kProtocolVersion, config, role, 0};
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
        firstToken = welcomeIn(repliesOf(first)).reconnectToken;
        secondToken = welcomeIn(repliesOf(second)).reconnectToken;
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
    std::uint64_t firstToken = 0;
    std::uint64_t secondToken = 0;
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

TEST_CASE("a player who has left no longer holds up the frames of the players who stay")
{
    Match match;
    match.relay.core.peerLeft(match.second.id());

    match.sendInputs(match.first, 1, inputOf(10));

    const Replies replies = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);
    const std::array<std::byte, 3> droppedNeutral{std::byte{2}, std::byte{0}, std::byte{0}};
    REQUIRE(confirmations.size() == 1U);
    REQUIRE(std::ranges::equal(slotsOfFrame(confirmations[0], 1).subspan(3, 3), droppedNeutral));
}

TEST_CASE("a relay core is empty once everyone in it has left and the grace of their slots has passed")
{
    Match match;
    match.relay.core.peerLeft(match.first.id());
    match.relay.core.peerLeft(match.second.id());
    const bool isEmptyWithinTheGrace = match.relay.core.isEmpty();

    match.relay.clock.advance(unison::net::RelaySettings{}.reconnectGraceMicroseconds);
    match.relay.core.update();

    REQUIRE_FALSE(isEmptyWithinTheGrace);
    REQUIRE(match.relay.core.isEmpty());
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

TEST_CASE("a pong carries the frame the relay's clock has due, counted from the first input to arrive")
{
    Match match;
    const std::array<std::byte, 10> fiveFrames{};
    match.sendInputs(match.first, 1, fiveFrames);
    static_cast<void>(Match::repliesOf(match.first));
    match.relay.clock.advance(100'000);

    sendMessage(match.first, match.relay.endpoint.id(), unison::net::Ping{7});
    match.relay.endpoint.poll(match.relay.core);

    const unison::net::Pong pong = onlyReplyAs<unison::net::Pong>(Match::repliesOf(match.first));
    REQUIRE(pong.confirmedFrame == 0U);
    REQUIRE(pong.dueFrame == 5U + 6U);
}

TEST_CASE("a pong before the first input of the match names no frame due")
{
    Match match;

    sendMessage(match.first, match.relay.endpoint.id(), unison::net::Ping{7});
    match.relay.endpoint.poll(match.relay.core);

    const unison::net::Pong pong = onlyReplyAs<unison::net::Pong>(Match::repliesOf(match.first));
    REQUIRE(pong.dueFrame == 0U);
}

namespace
{

struct RunningMatch
{
    RunningMatch()
        : endpoint{hub.join()},
          core{endpoint, clock, threeSlotsOfTwoBytes(), unison::net::RelaySettings{}, &roundTrips}, first{hub.join()},
          second{hub.join()}, joiner{hub.join()}
    {
        hello(first);
        hello(second);
        sendInputs(first, 1, inputOf(1));
        sendInputs(second, 1, inputOf(2));
        firstToken = welcomeIn(Match::repliesOf(first)).reconnectToken;
        secondToken = welcomeIn(Match::repliesOf(second)).reconnectToken;
    }

    void hello(unison::net::LoopbackEndpoint& client)
    {
        sendMessage(client, endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
        endpoint.poll(core);
    }

    void sendInputs(unison::net::LoopbackEndpoint& client, std::uint32_t frame, std::span<const std::byte> inputs)
    {
        sendMessage(client, endpoint.id(), unison::net::Input{frame, 2, 1, inputs});
        endpoint.poll(core);
    }

    void leaveForGood(unison::net::LoopbackEndpoint& client)
    {
        core.peerLeft(client.id());
        clock.advance(unison::net::RelaySettings{}.reconnectGraceMicroseconds);
        core.update();
    }

    unison::net::LoopbackHub hub;
    unison::net::ManualClock clock;
    unison::test::FixedRoundTrips roundTrips;
    unison::net::LoopbackEndpoint& endpoint;
    unison::net::RelayCore core;
    unison::net::LoopbackEndpoint& first;
    unison::net::LoopbackEndpoint& second;
    unison::net::LoopbackEndpoint& joiner;
    std::uint64_t firstToken = 0;
    std::uint64_t secondToken = 0;
};

std::vector<unison::net::SnapshotRequest> snapshotRequestsIn(const Replies& replies)
{
    std::vector<unison::net::SnapshotRequest> requests;

    for (const std::vector<std::byte>& reply : replies)
    {
        const auto decoded = unison::net::decode(reply);

        if (decoded.has_value() && std::holds_alternative<unison::net::SnapshotRequest>(*decoded))
        {
            requests.push_back(std::get<unison::net::SnapshotRequest>(*decoded));
        }
    }

    return requests;
}

}

TEST_CASE("a player joining a running room makes the relay ask the player with the lowest round trip for a snapshot")
{
    RunningMatch match;
    match.roundTrips.set(match.first.id(), 40'000);
    match.roundTrips.set(match.second.id(), 10'000);

    match.hello(match.joiner);

    const std::vector<unison::net::SnapshotRequest> toSecond = snapshotRequestsIn(Match::repliesOf(match.second));
    REQUIRE(snapshotRequestsIn(Match::repliesOf(match.first)).empty());
    REQUIRE(toSecond.size() == 1U);
    REQUIRE(toSecond.front().frame == 2U);
}

TEST_CASE("the relay asks the player in the lowest slot for a snapshot when no round trip is measured")
{
    RunningMatch match;

    match.hello(match.joiner);

    REQUIRE(snapshotRequestsIn(Match::repliesOf(match.first)).size() == 1U);
    REQUIRE(snapshotRequestsIn(Match::repliesOf(match.second)).empty());
}

namespace
{

std::vector<std::byte> snapshotBytes(std::size_t size)
{
    std::vector<std::byte> bytes(size);

    for (std::size_t index = 0; index < size; ++index)
    {
        bytes[index] = static_cast<std::byte>(index);
    }

    return bytes;
}

template <typename T>
std::vector<T> allOf(const Replies& replies)
{
    std::vector<T> found;

    for (const std::vector<std::byte>& reply : replies)
    {
        const auto decoded = unison::net::decode(reply);

        if (decoded.has_value() && std::holds_alternative<T>(*decoded))
        {
            found.push_back(std::get<T>(*decoded));
        }
    }

    return found;
}

std::vector<std::uint32_t> framesConfirmedIn(const Replies& replies)
{
    std::vector<std::uint32_t> frames;

    for (const unison::net::Confirmed& confirmed : allOf<unison::net::Confirmed>(replies))
    {
        for (std::uint32_t frame = confirmed.firstFrame; frame < confirmed.firstFrame + confirmed.frameCount; ++frame)
        {
            frames.push_back(frame);
        }
    }

    std::ranges::sort(frames);
    const auto repeats = std::ranges::unique(frames);
    frames.erase(repeats.begin(), repeats.end());

    return frames;
}

}

TEST_CASE("a player joining a running room holds its slot while frames are confirmed without it")
{
    RunningMatch match;
    match.hello(match.joiner);

    match.sendInputs(match.first, 2, inputOf(3));
    match.sendInputs(match.second, 2, inputOf(4));

    REQUIRE(framesConfirmedIn(Match::repliesOf(match.first)).back() == 2U);
}

TEST_CASE("the relay welcomes a joiner at the snapshot's frame before handing it the donor's chunks in order")
{
    RunningMatch match;
    match.hello(match.joiner);
    match.sendInputs(match.first, 2, inputOf(3));
    match.sendInputs(match.second, 2, inputOf(4));
    static_cast<void>(Match::repliesOf(match.joiner));
    const std::vector<std::byte> snapshot = snapshotBytes(2 * unison::net::snapshotBytesPerChunk());
    const unison::net::SnapshotChunk firstChunk{
        2, 0, 2, std::span{snapshot}.first(unison::net::snapshotBytesPerChunk())};
    const unison::net::SnapshotChunk secondChunk{
        2, 1, 2, std::span{snapshot}.subspan(unison::net::snapshotBytesPerChunk())};

    sendMessage(match.first, match.endpoint.id(), firstChunk);
    sendMessage(match.first, match.endpoint.id(), secondChunk);
    match.endpoint.poll(match.core);

    const Replies atJoiner = Match::repliesOf(match.joiner);
    const std::vector<unison::net::Welcome> welcomes = allOf<unison::net::Welcome>(atJoiner);
    const std::vector<unison::net::SnapshotChunk> chunks = allOf<unison::net::SnapshotChunk>(atJoiner);
    REQUIRE(welcomes.size() == 1U);
    REQUIRE(welcomes.front().slot == 2U);
    REQUIRE(welcomes.front().startFrame == 2U);
    REQUIRE(chunks.size() == 2U);
    REQUIRE(chunks[0].chunkIndex == 0U);
    REQUIRE(chunks[1].chunkIndex == 1U);
    REQUIRE(std::holds_alternative<unison::net::Welcome>(*unison::net::decode(atJoiner.front())));
}

TEST_CASE("a joiner hears of every frame after the snapshot's, those confirmed before the snapshot came included")
{
    RunningMatch match;
    match.hello(match.joiner);
    for (std::uint32_t frame = 2; frame <= 8; ++frame)
    {
        match.sendInputs(match.first, frame, inputOf(3));
        match.sendInputs(match.second, frame, inputOf(4));
    }
    static_cast<void>(Match::repliesOf(match.joiner));
    const std::vector<std::byte> snapshot = snapshotBytes(10);

    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{2, 0, 1, snapshot});
    match.endpoint.poll(match.core);
    match.sendInputs(match.first, 9, inputOf(5));
    match.sendInputs(match.second, 9, inputOf(6));

    REQUIRE(std::ranges::includes(framesConfirmedIn(Match::repliesOf(match.joiner)), std::views::iota(3U, 10U)));
}

TEST_CASE("a joiner's slot is confirmed absent until the first frame it sends an input for, and present from then on")
{
    RunningMatch match;
    match.hello(match.joiner);
    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);
    match.sendInputs(match.first, 2, inputOf(3));
    match.sendInputs(match.second, 2, inputOf(4));
    static_cast<void>(Match::repliesOf(match.first));

    match.sendInputs(match.joiner, 3, inputOf(7));
    match.sendInputs(match.first, 3, inputOf(5));
    match.sendInputs(match.second, 3, inputOf(6));

    const Replies atFirst = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = allOf<unison::net::Confirmed>(atFirst);
    REQUIRE_FALSE(confirmations.empty());
    const unison::net::Confirmed& newest = confirmations.back();
    const std::size_t frameSize = unison::net::confirmedFrameSize(newest.slotCount, newest.inputSize);
    const auto flagsOf = [&newest, frameSize](std::uint32_t frame, std::size_t slot)
    {
        const std::size_t at = std::size_t{frame - newest.firstFrame} * frameSize + slot * (1U + newest.inputSize);
        return std::to_integer<std::uint8_t>(newest.slots[at]);
    };
    REQUIRE(newest.firstFrame <= 2U);
    REQUIRE(newest.firstFrame + newest.frameCount - 1U == 3U);
    REQUIRE(flagsOf(2, 2) == 0U);
    REQUIRE(flagsOf(3, 2) == static_cast<std::uint8_t>(unison::net::SlotFlags::Present));
}

TEST_CASE("the relay hands one snapshot to every player joining through the same donor")
{
    RunningMatch match;
    unison::net::LoopbackEndpoint& lateComer = match.hub.join();
    match.leaveForGood(match.second);
    match.hello(match.joiner);
    match.hello(lateComer);

    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);

    const Replies atJoiner = Match::repliesOf(match.joiner);
    const Replies atLateComer = Match::repliesOf(lateComer);
    REQUIRE(allOf<unison::net::Welcome>(atJoiner).size() == 1U);
    REQUIRE(allOf<unison::net::SnapshotChunk>(atJoiner).size() == 1U);
    REQUIRE(allOf<unison::net::Welcome>(atLateComer).size() == 1U);
    REQUIRE(allOf<unison::net::SnapshotChunk>(atLateComer).size() == 1U);
}

TEST_CASE("a player joining while a snapshot is on its way is handed the next snapshot whole")
{
    RunningMatch match;
    unison::net::LoopbackEndpoint& lateComer = match.hub.join();
    match.leaveForGood(match.second);
    match.sendInputs(match.first, 2, inputOf(3));
    match.sendInputs(match.first, 3, inputOf(4));
    match.hello(match.joiner);
    const std::vector<std::byte> snapshot = snapshotBytes(2 * unison::net::snapshotBytesPerChunk());
    const auto chunkOf = [&snapshot](std::uint32_t frame, std::uint32_t index)
    {
        const std::size_t size = unison::net::snapshotBytesPerChunk();
        return unison::net::SnapshotChunk{frame, index, 2, std::span{snapshot}.subspan(index * size, size)};
    };
    sendMessage(match.first, match.endpoint.id(), chunkOf(2, 0));
    match.endpoint.poll(match.core);
    match.hello(lateComer);

    sendMessage(match.first, match.endpoint.id(), chunkOf(2, 1));
    sendMessage(match.first, match.endpoint.id(), chunkOf(3, 0));
    sendMessage(match.first, match.endpoint.id(), chunkOf(3, 1));
    match.endpoint.poll(match.core);

    const Replies atLateComer = Match::repliesOf(lateComer);
    const std::vector<unison::net::Welcome> welcomes = allOf<unison::net::Welcome>(atLateComer);
    const std::vector<unison::net::SnapshotChunk> chunks = allOf<unison::net::SnapshotChunk>(atLateComer);
    REQUIRE(welcomes.size() == 1U);
    REQUIRE(welcomes.front().startFrame == 3U);
    REQUIRE(chunks.size() == 2U);
    REQUIRE(std::ranges::all_of(chunks, [](const unison::net::SnapshotChunk& chunk) { return chunk.frame == 3U; }));
}

TEST_CASE("a snapshot of a frame the relay has not confirmed is not handed to a joiner")
{
    const std::uint32_t unconfirmed = GENERATE(0U, 2U, 0xFFFFFFFFU);
    RunningMatch match;
    match.hello(match.joiner);

    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{unconfirmed, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);

    const Replies atJoiner = Match::repliesOf(match.joiner);
    REQUIRE(allOf<unison::net::Welcome>(atJoiner).empty());
    REQUIRE(allOf<unison::net::SnapshotChunk>(atJoiner).empty());
}

TEST_CASE("a player still joining is never asked for a snapshot")
{
    RunningMatch match;
    unison::net::LoopbackEndpoint& lateComer = match.hub.join();
    match.leaveForGood(match.second);
    match.roundTrips.set(match.first.id(), 40'000);
    match.roundTrips.set(match.joiner.id(), 10'000);
    match.hello(match.joiner);

    match.hello(lateComer);

    REQUIRE(snapshotRequestsIn(Match::repliesOf(match.first)).size() == 2U);
    REQUIRE(snapshotRequestsIn(Match::repliesOf(match.joiner)).empty());
}

TEST_CASE("a joiner's first input naming frames confirmed without it puts its slot in play from the next frame only")
{
    RunningMatch match;
    match.hello(match.joiner);
    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);
    match.sendInputs(match.first, 2, inputOf(3));
    match.sendInputs(match.second, 2, inputOf(4));
    const std::array<std::byte, 4> framesTwoAndThree{};
    sendMessage(match.joiner, match.endpoint.id(), unison::net::Input{2, 2, 2, framesTwoAndThree});
    match.endpoint.poll(match.core);

    sendMessage(match.first, match.endpoint.id(), unison::net::Checksum{2, 77});
    sendMessage(match.second, match.endpoint.id(), unison::net::Checksum{2, 77});
    sendMessage(match.joiner, match.endpoint.id(), unison::net::Checksum{2, 78});
    match.endpoint.poll(match.core);

    REQUIRE(allOf<unison::net::Desync>(Match::repliesOf(match.first)).empty());
}

TEST_CASE("a joiner whose donor leaves before the last chunk has its snapshot asked of another player in play")
{
    RunningMatch match;
    match.hello(match.joiner);
    const std::vector<std::byte> snapshot = snapshotBytes(2 * unison::net::snapshotBytesPerChunk());
    sendMessage(match.first,
                match.endpoint.id(),
                unison::net::SnapshotChunk{1, 0, 2, std::span{snapshot}.first(unison::net::snapshotBytesPerChunk())});
    match.endpoint.poll(match.core);
    static_cast<void>(Match::repliesOf(match.second));

    match.core.peerLeft(match.first.id());

    const std::vector<unison::net::SnapshotRequest> toSecond = snapshotRequestsIn(Match::repliesOf(match.second));
    REQUIRE(toSecond.size() == 1U);
    REQUIRE(toSecond.front().frame == 2U);
}

TEST_CASE("a joiner whose donor left mid-snapshot is welcomed again with the next donor's whole snapshot")
{
    RunningMatch match;
    match.hello(match.joiner);
    const std::vector<std::byte> snapshot = snapshotBytes(2 * unison::net::snapshotBytesPerChunk());
    const auto chunkOf = [&snapshot](std::uint32_t frame, std::uint32_t index)
    {
        const std::size_t size = unison::net::snapshotBytesPerChunk();
        return unison::net::SnapshotChunk{frame, index, 2, std::span{snapshot}.subspan(index * size, size)};
    };
    sendMessage(match.first, match.endpoint.id(), chunkOf(1, 0));
    match.endpoint.poll(match.core);
    match.core.peerLeft(match.first.id());
    match.sendInputs(match.second, 2, inputOf(4));
    static_cast<void>(Match::repliesOf(match.joiner));

    sendMessage(match.second, match.endpoint.id(), chunkOf(2, 0));
    sendMessage(match.second, match.endpoint.id(), chunkOf(2, 1));
    match.endpoint.poll(match.core);

    const Replies atJoiner = Match::repliesOf(match.joiner);
    const std::vector<unison::net::Welcome> welcomes = allOf<unison::net::Welcome>(atJoiner);
    const std::vector<unison::net::SnapshotChunk> chunks = allOf<unison::net::SnapshotChunk>(atJoiner);
    REQUIRE(welcomes.size() == 1U);
    REQUIRE(welcomes.front().startFrame == 2U);
    REQUIRE(chunks.size() == 2U);
    REQUIRE(std::ranges::all_of(chunks, [](const unison::net::SnapshotChunk& chunk) { return chunk.frame == 2U; }));
}

TEST_CASE("a joiner that leaves before its snapshot comes is handed nothing of it")
{
    RunningMatch match;
    match.hello(match.joiner);
    match.core.peerLeft(match.joiner.id());
    static_cast<void>(Match::repliesOf(match.joiner));

    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);

    const Replies atJoiner = Match::repliesOf(match.joiner);
    REQUIRE(allOf<unison::net::Welcome>(atJoiner).empty());
    REQUIRE(allOf<unison::net::SnapshotChunk>(atJoiner).empty());
}

TEST_CASE("a player leaving while no joiner waits for its snapshot makes the relay ask nobody for one")
{
    RunningMatch match;

    match.core.peerLeft(match.second.id());

    REQUIRE(snapshotRequestsIn(Match::repliesOf(match.first)).empty());
}

TEST_CASE("a second hello from a member changes nothing")
{
    Match match;

    sendMessage(match.first, match.relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    match.relay.endpoint.poll(match.relay.core);
    const Replies atFirst = Match::repliesOf(match.first);
    const Replies newcomer = repliesTo(match.relay, helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));

    REQUIRE(atFirst.empty());
    REQUIRE(onlyReplyAs<unison::net::Welcome>(newcomer).slot == 2U);
}

TEST_CASE("a player is welcomed with a reconnect token and a spectator without one")
{
    Relay relay{twoPlayers()};

    const Replies player = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player));
    const Replies spectator = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Spectator));

    REQUIRE(onlyReplyAs<unison::net::Welcome>(player).reconnectToken != 0U);
    REQUIRE(onlyReplyAs<unison::net::Welcome>(spectator).reconnectToken == 0U);
}

TEST_CASE("every player of a room is welcomed with a reconnect token of its own")
{
    Relay relay{twoPlayers()};

    const Replies first = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player));
    const Replies second = repliesTo(relay, helloFor(twoPlayers(), unison::net::Role::Player));

    REQUIRE(onlyReplyAs<unison::net::Welcome>(first).reconnectToken !=
            onlyReplyAs<unison::net::Welcome>(second).reconnectToken);
}

TEST_CASE("a late joiner is welcomed with a reconnect token")
{
    RunningMatch match;
    match.hello(match.joiner);

    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);

    const std::vector<unison::net::Welcome> welcomes = allOf<unison::net::Welcome>(Match::repliesOf(match.joiner));
    REQUIRE(welcomes.size() == 1U);
    REQUIRE(welcomes.front().reconnectToken != 0U);
}

TEST_CASE("the slot of a player who left is held, the frames after it confirmed with its last input dropped")
{
    Match match;
    match.sendInputs(match.first, 1, inputOf(3));
    match.sendInputs(match.second, 1, inputOf(4));
    static_cast<void>(Match::repliesOf(match.first));
    match.relay.core.peerLeft(match.second.id());

    match.sendInputs(match.first, 2, inputOf(5));

    const Replies replies = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);
    const std::array<std::byte, 3> repeated{std::byte{2}, std::byte{4}, std::byte{4}};
    REQUIRE(confirmations.size() == 1U);
    REQUIRE(newestFrameOf(confirmations[0]) == 2U);
    REQUIRE(std::ranges::equal(slotsOfFrame(confirmations[0], 2).subspan(3, 3), repeated));
}

TEST_CASE("a slot held for a player who left is given to nobody else")
{
    Match match;
    match.relay.core.peerLeft(match.second.id());

    const Replies newcomer = repliesTo(match.relay, helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));

    REQUIRE(onlyReplyAs<unison::net::Welcome>(newcomer).slot == 2U);
}

TEST_CASE("a held slot is still held a microsecond before its grace has passed")
{
    Match match;
    match.relay.core.peerLeft(match.second.id());

    match.relay.clock.advance(unison::net::RelaySettings{}.reconnectGraceMicroseconds - 1);
    match.relay.core.update();

    const Replies newcomer = repliesTo(match.relay, helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    REQUIRE(onlyReplyAs<unison::net::Welcome>(newcomer).slot == 2U);
}

TEST_CASE("a held slot is free for a newcomer once its grace has passed")
{
    Match match;
    match.relay.core.peerLeft(match.second.id());

    match.relay.clock.advance(unison::net::RelaySettings{}.reconnectGraceMicroseconds);
    match.relay.core.update();

    const Replies newcomer = repliesTo(match.relay, helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    REQUIRE(onlyReplyAs<unison::net::Welcome>(newcomer).slot == 1U);
}

TEST_CASE("a held slot is confirmed absent once its grace has passed")
{
    Match match;
    match.sendInputs(match.first, 1, inputOf(3));
    match.sendInputs(match.second, 1, inputOf(4));
    match.relay.core.peerLeft(match.second.id());
    match.relay.clock.advance(unison::net::RelaySettings{}.reconnectGraceMicroseconds);
    match.relay.core.update();
    static_cast<void>(Match::repliesOf(match.first));

    match.sendInputs(match.first, 2, inputOf(5));

    const Replies replies = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);
    const std::array<std::byte, 3> absent{std::byte{0}, std::byte{0}, std::byte{0}};
    REQUIRE(confirmations.size() == 1U);
    REQUIRE(std::ranges::equal(slotsOfFrame(confirmations[0], 2).subspan(3, 3), absent));
}

TEST_CASE("the checksums of the players who stay are judged without the player whose slot is held")
{
    Match match;
    unison::net::LoopbackEndpoint& third = match.relay.hub.join();
    sendMessage(third, match.relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    match.relay.endpoint.poll(match.relay.core);
    match.relay.core.peerLeft(third.id());
    static_cast<void>(Match::repliesOf(match.first));

    sendMessage(match.first, match.relay.endpoint.id(), unison::net::Checksum{20, 77});
    sendMessage(match.second, match.relay.endpoint.id(), unison::net::Checksum{20, 78});
    match.relay.endpoint.poll(match.relay.core);

    const std::vector<unison::net::Desync> desyncs = desyncsIn(Match::repliesOf(match.first));
    REQUIRE(desyncs.size() == 1U);
    REQUIRE(desyncs.front().minoritySlots == 0b11U);
}

TEST_CASE("a player whose peer has gone is sent nothing while its slot is held")
{
    Match match;
    match.relay.core.peerLeft(match.second.id());
    static_cast<void>(Match::repliesOf(match.second));

    match.sendInputs(match.first, 1, inputOf(3));

    REQUIRE_FALSE(confirmationsIn(Match::repliesOf(match.first)).empty());
    REQUIRE(Match::repliesOf(match.second).empty());
}

namespace
{

unison::net::Hello helloBackWith(std::uint64_t reconnectToken)
{
    unison::net::Hello hello = helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player);
    hello.reconnectToken = reconnectToken;

    return hello;
}

}

TEST_CASE("a player back with its token before the match has started is welcomed into its slot at frame nought")
{
    Match match;
    match.relay.core.peerLeft(match.second.id());
    unison::net::LoopbackEndpoint& back = match.relay.hub.join();

    sendMessage(back, match.relay.endpoint.id(), helloBackWith(match.secondToken));
    match.relay.endpoint.poll(match.relay.core);

    const unison::net::Welcome welcome = onlyReplyAs<unison::net::Welcome>(Match::repliesOf(back));
    REQUIRE(welcome.slot == 1U);
    REQUIRE(welcome.startFrame == 0U);
    REQUIRE(welcome.reconnectToken == match.secondToken);
}

TEST_CASE("a player back with its token in a running match is caught up into its slot from a donor's snapshot")
{
    RunningMatch match;
    match.core.peerLeft(match.second.id());
    unison::net::LoopbackEndpoint& back = match.hub.join();
    sendMessage(back, match.endpoint.id(), helloBackWith(match.secondToken));
    match.endpoint.poll(match.core);
    const std::vector<unison::net::SnapshotRequest> toFirst = snapshotRequestsIn(Match::repliesOf(match.first));

    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);

    const unison::net::Welcome welcome = welcomeIn(Match::repliesOf(back));
    REQUIRE(toFirst.size() == 1U);
    REQUIRE(welcome.slot == 1U);
    REQUIRE(welcome.startFrame == 1U);
    REQUIRE(welcome.reconnectToken == match.secondToken);
}

TEST_CASE("a player back from a drop is not waited for until its first input, its last input dropped meanwhile")
{
    RunningMatch match;
    match.core.peerLeft(match.second.id());
    unison::net::LoopbackEndpoint& back = match.hub.join();
    sendMessage(back, match.endpoint.id(), helloBackWith(match.secondToken));
    match.endpoint.poll(match.core);
    static_cast<void>(Match::repliesOf(match.first));

    match.sendInputs(match.first, 2, inputOf(5));

    const Replies replies = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);
    const std::array<std::byte, 3> repeated{std::byte{2}, std::byte{2}, std::byte{2}};
    REQUIRE(confirmations.size() == 1U);
    REQUIRE(newestFrameOf(confirmations[0]) == 2U);
    REQUIRE(std::ranges::equal(slotsOfFrame(confirmations[0], 2).subspan(3, 3), repeated));
}

TEST_CASE("a player back from a drop is waited for again from the first frame it sends an input for")
{
    RunningMatch match;
    match.core.peerLeft(match.second.id());
    unison::net::LoopbackEndpoint& back = match.hub.join();
    sendMessage(back, match.endpoint.id(), helloBackWith(match.secondToken));
    match.endpoint.poll(match.core);
    match.sendInputs(back, 2, inputOf(7));
    static_cast<void>(Match::repliesOf(match.first));

    match.sendInputs(match.first, 2, inputOf(5));
    match.sendInputs(match.first, 3, inputOf(6));

    const Replies replies = Match::repliesOf(match.first);
    const std::vector<unison::net::Confirmed> confirmations = confirmationsIn(replies);
    const std::array<std::byte, 3> present{std::byte{1}, std::byte{7}, std::byte{7}};
    REQUIRE(confirmations.size() == 1U);
    REQUIRE(newestFrameOf(confirmations[0]) == 2U);
    REQUIRE(std::ranges::equal(slotsOfFrame(confirmations[0], 2).subspan(3, 3), present));
}

TEST_CASE("a player catching up after a drop is never asked for a snapshot")
{
    RunningMatch match;
    match.core.peerLeft(match.second.id());
    unison::net::LoopbackEndpoint& back = match.hub.join();
    match.roundTrips.set(match.first.id(), 40'000);
    match.roundTrips.set(back.id(), 10'000);
    sendMessage(back, match.endpoint.id(), helloBackWith(match.secondToken));
    match.endpoint.poll(match.core);

    match.hello(match.joiner);

    REQUIRE(snapshotRequestsIn(Match::repliesOf(match.first)).size() == 2U);
    REQUIRE(snapshotRequestsIn(Match::repliesOf(back)).empty());
}

TEST_CASE("a hello with a token nobody holds is seated as any other player's")
{
    Match match;
    unison::net::LoopbackEndpoint& stranger = match.relay.hub.join();

    sendMessage(stranger, match.relay.endpoint.id(), helloBackWith(12345));
    match.relay.endpoint.poll(match.relay.core);

    REQUIRE(onlyReplyAs<unison::net::Welcome>(Match::repliesOf(stranger)).slot == 2U);
}

TEST_CASE("a player back with its token takes its slot over from an old peer the relay has not seen go")
{
    Match match;
    unison::net::LoopbackEndpoint& back = match.relay.hub.join();
    sendMessage(back, match.relay.endpoint.id(), helloBackWith(match.secondToken));
    match.relay.endpoint.poll(match.relay.core);
    const unison::net::Welcome welcome = onlyReplyAs<unison::net::Welcome>(Match::repliesOf(back));

    match.sendInputs(match.first, 1, inputOf(3));
    match.sendInputs(back, 1, inputOf(4));

    REQUIRE(welcome.slot == 1U);
    REQUIRE(Match::repliesOf(match.second).empty());
    REQUIRE_FALSE(confirmationsIn(Match::repliesOf(back)).empty());
}

TEST_CASE("a spectator joining a running match is caught up from a donor's snapshot, taking no slot")
{
    RunningMatch match;
    unison::net::LoopbackEndpoint& spectator = match.hub.join();
    sendMessage(spectator, match.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Spectator));
    match.endpoint.poll(match.core);
    const std::vector<unison::net::SnapshotRequest> toFirst = snapshotRequestsIn(Match::repliesOf(match.first));

    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);

    const Replies atSpectator = Match::repliesOf(spectator);
    const unison::net::Welcome welcome = welcomeIn(atSpectator);
    REQUIRE(toFirst.size() == 1U);
    REQUIRE(welcome.slot == unison::net::kNoSlot);
    REQUIRE(welcome.startFrame == 1U);
    REQUIRE(welcome.reconnectToken == 0U);
    REQUIRE(allOf<unison::net::SnapshotChunk>(atSpectator).size() == 1U);
}

TEST_CASE("a spectator joining a running match leaves every slot to the players")
{
    RunningMatch match;
    unison::net::LoopbackEndpoint& spectator = match.hub.join();
    sendMessage(spectator, match.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Spectator));
    match.endpoint.poll(match.core);
    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);

    match.hello(match.joiner);
    sendMessage(match.first, match.endpoint.id(), unison::net::SnapshotChunk{1, 0, 1, snapshotBytes(10)});
    match.endpoint.poll(match.core);

    REQUIRE(welcomeIn(Match::repliesOf(match.joiner)).slot == 2U);
}

TEST_CASE("a spectator is never asked for a snapshot, however near it is")
{
    unison::test::FixedRoundTrips roundTrips;
    Relay relay{threeSlotsOfTwoBytes(), &roundTrips};
    unison::net::LoopbackEndpoint& player = relay.hub.join();
    unison::net::LoopbackEndpoint& spectator = relay.hub.join();
    unison::net::LoopbackEndpoint& joiner = relay.hub.join();
    sendMessage(player, relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    sendMessage(spectator, relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Spectator));
    relay.endpoint.poll(relay.core);
    roundTrips.set(player.id(), 40'000);
    roundTrips.set(spectator.id(), 1'000);
    sendMessage(player, relay.endpoint.id(), unison::net::Input{1, 2, 1, inputOf(1)});
    relay.endpoint.poll(relay.core);

    sendMessage(joiner, relay.endpoint.id(), helloFor(threeSlotsOfTwoBytes(), unison::net::Role::Player));
    relay.endpoint.poll(relay.core);

    REQUIRE(snapshotRequestsIn(Match::repliesOf(player)).size() == 1U);
    REQUIRE(snapshotRequestsIn(Match::repliesOf(spectator)).empty());
}
