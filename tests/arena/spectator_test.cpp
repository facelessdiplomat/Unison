#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <support/checksum_tap.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/relay_core.hpp>
#include <unison/session/networked_session.hpp>
#include <unison/session/verified_frame_receiver.hpp>

#include <cstddef>
#include <cstdint>
#include <map>

namespace
{

constexpr std::uint8_t kPlayers = 2;
constexpr std::uint64_t kTickMicroseconds = 16'667;
constexpr std::uint32_t kHostFrames = 300;

unison::net::SessionConfig twoPlayerMatch()
{
    unison::net::SessionConfig config;
    config.slotCount = kPlayers;
    config.inputSize = sizeof(arena::ArenaInput);
    config.checksumInterval = 1;

    return config;
}

class ChecksumsHeard final : public unison::session::IVerifiedFrameReceiver
{
public:
    void frameVerified(const unison::session::VerifiedFrame& frame) override
    {
        if (frame.checksum.has_value())
        {
            byFrame.emplace(frame.frameNumber, *frame.checksum);
        }
    }

    std::map<std::uint32_t, std::uint64_t> byFrame;
};

void playHostFrame(unison::session::NetworkedSession& client,
                   const unison::sim::FrameInputs& scripted,
                   std::size_t slot,
                   std::uint64_t now)
{
    client.update(now);

    const std::int32_t ticks = 1 + client.takeTickCorrection();

    for (std::int32_t tick = 0; tick < ticks; ++tick)
    {
        client.setLocalInput(scripted.bytesAt(slot).first(sizeof(arena::ArenaInput)));
        client.tick();
    }
}

struct Outcome
{
    bool wasAlwaysVerified = true;
    std::uint32_t spectatorVerified = 0;
    std::uint32_t firstVerified = 0;
    std::map<std::uint32_t, std::uint64_t> playerReports;
    std::map<std::uint32_t, std::uint64_t> spectatorReports;
};

Outcome watchAMatch(std::uint32_t spectatorComesAt, std::uint32_t delayFrames)
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& relayEnd = hub.join();
    unison::net::LoopbackEndpoint& firstEnd = hub.join();
    unison::net::LoopbackEndpoint& secondEnd = hub.join();
    unison::net::LoopbackEndpoint& spectatorEnd = hub.join();
    unison::net::ManualClock clock;
    unison::net::RelayCore relay{relayEnd, clock, twoPlayerMatch()};
    unison::test::ChecksumTap tap{relay, firstEnd.id(), secondEnd.id()};
    arena::ArenaSimulation firstMatch{kPlayers};
    arena::ArenaSimulation secondMatch{kPlayers};
    arena::ArenaSimulation watchedMatch{kPlayers};
    ChecksumsHeard heard;
    unison::session::NetworkedSession first{
        firstMatch.frame(), firstMatch.pipeline(), twoPlayerMatch(), firstEnd, relayEnd.id()};
    unison::session::NetworkedSession second{
        secondMatch.frame(), secondMatch.pipeline(), twoPlayerMatch(), secondEnd, relayEnd.id()};
    unison::session::NetworkedSession spectator{
        watchedMatch.frame(), watchedMatch.pipeline(), twoPlayerMatch(), spectatorEnd, relayEnd.id(), 0, &heard};
    first.join();
    second.join();
    Outcome outcome;

    for (std::uint32_t hostFrame = 0; hostFrame < kHostFrames; ++hostFrame)
    {
        const std::uint64_t now = std::uint64_t{hostFrame} * kTickMicroseconds;

        if (hostFrame == spectatorComesAt)
        {
            spectator.spectate(delayFrames);
        }

        relayEnd.poll(tap);
        relay.update();

        const unison::sim::FrameInputs scripted = unison::test::scriptedInputs(hostFrame);
        playHostFrame(first, scripted, 0, now);
        playHostFrame(second, scripted, 1, now);
        playHostFrame(spectator, scripted, 0, now);

        const unison::session::Session* watched = spectator.session();
        outcome.wasAlwaysVerified =
            outcome.wasAlwaysVerified && (watched == nullptr || watched->predictedFrame() == watched->verifiedFrame());

        clock.advance(kTickMicroseconds);
    }

    relayEnd.poll(tap);

    outcome.spectatorVerified = spectator.session() == nullptr ? 0 : spectator.session()->verifiedFrame();
    outcome.firstVerified = first.session()->verifiedFrame();
    outcome.playerReports = {tap.firstReports.begin(), tap.firstReports.end()};
    outcome.spectatorReports = heard.byFrame;

    return outcome;
}

std::uint32_t framesAgreed(const Outcome& outcome)
{
    std::uint32_t agreed = 0;

    for (const auto& [frame, checksum] : outcome.spectatorReports)
    {
        const auto played = outcome.playerReports.find(frame);

        if (played != outcome.playerReports.end())
        {
            CAPTURE(frame);
            REQUIRE(played->second == checksum);
            ++agreed;
        }
    }

    return agreed;
}

}

TEST_CASE("a spectator of an arena match plays no frame ahead of the relay and agrees with the players on every one")
{
    const Outcome outcome = watchAMatch(0, 0);

    REQUIRE(outcome.wasAlwaysVerified);
    REQUIRE(outcome.spectatorVerified + 5U >= outcome.firstVerified);
    REQUIRE(framesAgreed(outcome) >= kHostFrames - 10U);
}

TEST_CASE("a spectator that comes into a running arena match late watches from a snapshot and agrees with the players")
{
    const Outcome outcome = watchAMatch(120, 0);

    REQUIRE(outcome.wasAlwaysVerified);
    REQUIRE(outcome.spectatorVerified + 5U >= outcome.firstVerified);
    REQUIRE(framesAgreed(outcome) >= kHostFrames - 120U - 20U);
}

TEST_CASE("a spectator with a delay watches that many frames behind the newest the relay confirmed")
{
    const Outcome outcome = watchAMatch(0, 10);

    REQUIRE(outcome.wasAlwaysVerified);
    REQUIRE(outcome.spectatorVerified + 10U <= outcome.firstVerified + 2U);
    REQUIRE(outcome.spectatorVerified + 16U >= outcome.firstVerified);
}
