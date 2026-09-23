#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <support/checksum_tap.hpp>
#include <support/outage_link.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/relay_core.hpp>
#include <unison/session/networked_session.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace
{

constexpr std::uint8_t kPlayers = 2;
constexpr std::uint64_t kTickMicroseconds = 16'667;
constexpr std::uint32_t kOutageStarts = 120;
constexpr std::uint32_t kOutageEnds = kOutageStarts + 30;
constexpr std::uint32_t kHostFrames = 600;

unison::net::SessionConfig twoPlayerMatch()
{
    unison::net::SessionConfig config;
    config.slotCount = kPlayers;
    config.inputSize = sizeof(arena::ArenaInput);
    config.checksumInterval = 1;

    return config;
}

struct Outcome
{
    bool cutOffClientStalled = false;
    std::uint32_t otherFrameWhenOutageEnded = 0;
    std::int64_t gapAtTheEnd = 0;
    unison::test::ChecksumReports otherReports;
    unison::test::ChecksumReports cutOffReports;
    bool anyDesync = false;
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

Outcome playThroughAHalfSecondOutage()
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& relayEnd = hub.join();
    unison::net::LoopbackEndpoint& otherEnd = hub.join();
    unison::net::LoopbackEndpoint& cutOffEnd = hub.join();
    unison::test::OutageLink cutOffLink{cutOffEnd};
    unison::net::ManualClock clock;
    unison::net::RelayCore relay{relayEnd, clock, twoPlayerMatch()};
    unison::test::ChecksumTap tap{relay, otherEnd.id(), cutOffEnd.id()};
    arena::ArenaSimulation otherMatch{kPlayers};
    arena::ArenaSimulation cutOffMatch{kPlayers};
    unison::session::NetworkedSession other{
        otherMatch.frame(), otherMatch.pipeline(), twoPlayerMatch(), otherEnd, relayEnd.id()};
    unison::session::NetworkedSession cutOff{
        cutOffMatch.frame(), cutOffMatch.pipeline(), twoPlayerMatch(), cutOffLink, relayEnd.id()};
    other.join();
    cutOff.join();

    Outcome outcome;

    for (std::uint32_t hostFrame = 0; hostFrame < kHostFrames; ++hostFrame)
    {
        const std::uint64_t now = std::uint64_t{hostFrame} * kTickMicroseconds;

        if (hostFrame == kOutageStarts)
        {
            cutOffLink.cut();
        }

        if (hostFrame == kOutageEnds)
        {
            cutOffLink.restore();
            outcome.otherFrameWhenOutageEnded = other.session()->predictedFrame();
        }

        relayEnd.poll(tap);
        relay.update();

        const unison::sim::FrameInputs scripted = unison::test::scriptedInputs(hostFrame);
        playHostFrame(other, scripted, 0, now);
        playHostFrame(cutOff, scripted, 1, now);

        const bool isInOutage = hostFrame >= kOutageStarts && hostFrame < kOutageEnds;
        outcome.cutOffClientStalled = outcome.cutOffClientStalled || (isInOutage && cutOff.session()->isStalled());

        clock.advance(kTickMicroseconds);
    }

    relayEnd.poll(tap);

    outcome.gapAtTheEnd = static_cast<std::int64_t>(other.session()->predictedFrame()) -
                          static_cast<std::int64_t>(cutOff.session()->predictedFrame());
    outcome.otherReports = tap.firstReports;
    outcome.cutOffReports = tap.secondReports;
    outcome.anyDesync = other.lastDesync().has_value() || cutOff.lastDesync().has_value();

    return outcome;
}

}

TEST_CASE("a client cut off for half a second stalls, then catches up without parting ways with the other")
{
    const Outcome outcome = playThroughAHalfSecondOutage();
    const std::size_t bothReported = std::min(outcome.otherReports.size(), outcome.cutOffReports.size());

    REQUIRE(outcome.cutOffClientStalled);
    REQUIRE(outcome.gapAtTheEnd >= -2);
    REQUIRE(outcome.gapAtTheEnd <= 2);
    REQUIRE(bothReported >= kHostFrames - 20);
    REQUIRE(unison::test::ChecksumReports(outcome.otherReports.begin(), outcome.otherReports.begin() + bothReported) ==
            unison::test::ChecksumReports(outcome.cutOffReports.begin(), outcome.cutOffReports.begin() + bothReported));
    REQUIRE_FALSE(outcome.anyDesync);
}

TEST_CASE("the other client plays on at its own pace while one is cut off")
{
    const Outcome outcome = playThroughAHalfSecondOutage();

    REQUIRE(outcome.otherFrameWhenOutageEnded + 2U >= kOutageEnds);
}
