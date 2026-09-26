#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <support/checksum_tap.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/relay_core.hpp>
#include <unison/session/networked_session.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>

namespace
{

constexpr std::uint8_t kPlayers = 2;
constexpr std::uint64_t kTickMicroseconds = 16'667;
constexpr std::uint32_t kDropsAt = 120;
constexpr std::uint32_t kComesBackAt = kDropsAt + 60;
constexpr std::uint32_t kHostFrames = 600;

unison::net::SessionConfig twoPlayerMatch()
{
    unison::net::SessionConfig config;
    config.slotCount = kPlayers;
    config.inputSize = sizeof(arena::ArenaInput);
    config.checksumInterval = 1;

    return config;
}

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
    std::uint8_t slotBeforeTheDrop = unison::net::kNoSlot;
    std::uint8_t slotAfterTheReturn = unison::net::kNoSlot;
    std::uint32_t startFrameAfterTheReturn = 0;
    std::uint32_t stayedVerified = 0;
    std::uint32_t returnedVerified = 0;
    std::map<std::uint32_t, std::uint64_t> stayedReports;
    std::map<std::uint32_t, std::uint64_t> droppedReports;
    bool anyDesync = false;
};

std::map<std::uint32_t, std::uint64_t> byFrame(const unison::test::ChecksumReports& reports)
{
    return {reports.begin(), reports.end()};
}

Outcome playThroughADropAndAReturn()
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& relayEnd = hub.join();
    unison::net::LoopbackEndpoint& stayedEnd = hub.join();
    unison::net::LoopbackEndpoint& droppedEnd = hub.join();
    unison::net::LoopbackEndpoint& returnedEnd = hub.join();
    unison::net::ManualClock clock;
    unison::net::RelayCore relay{relayEnd, clock, twoPlayerMatch()};
    unison::test::ChecksumTap tap{relay, stayedEnd.id(), droppedEnd.id()};
    arena::ArenaSimulation stayedMatch{kPlayers};
    arena::ArenaSimulation droppedMatch{kPlayers};
    arena::ArenaSimulation returnedMatch{kPlayers};
    unison::session::NetworkedSession stayed{
        stayedMatch.frame(), stayedMatch.pipeline(), twoPlayerMatch(), stayedEnd, relayEnd.id()};
    std::optional<unison::session::NetworkedSession> dropped;
    dropped.emplace(droppedMatch.frame(), droppedMatch.pipeline(), twoPlayerMatch(), droppedEnd, relayEnd.id());
    std::optional<unison::session::NetworkedSession> returned;
    stayed.join();
    dropped->join();

    Outcome outcome;
    std::uint64_t reconnectToken = 0;

    for (std::uint32_t hostFrame = 0; hostFrame < kHostFrames; ++hostFrame)
    {
        const std::uint64_t now = std::uint64_t{hostFrame} * kTickMicroseconds;

        if (hostFrame == kDropsAt)
        {
            outcome.slotBeforeTheDrop = dropped->localSlot();
            reconnectToken = dropped->reconnectToken();
            dropped.reset();
            relay.peerLeft(droppedEnd.id());
        }

        if (hostFrame == kComesBackAt)
        {
            returned.emplace(
                returnedMatch.frame(), returnedMatch.pipeline(), twoPlayerMatch(), returnedEnd, relayEnd.id());
            returned->join(reconnectToken);
        }

        relayEnd.poll(tap);
        relay.update();

        const unison::sim::FrameInputs scripted = unison::test::scriptedInputs(hostFrame);
        playHostFrame(stayed, scripted, 0, now);

        if (dropped.has_value())
        {
            playHostFrame(*dropped, scripted, 1, now);
        }

        if (returned.has_value())
        {
            playHostFrame(*returned, scripted, 1, now);
        }

        clock.advance(kTickMicroseconds);
    }

    relayEnd.poll(tap);

    outcome.slotAfterTheReturn = returned->localSlot();
    outcome.startFrameAfterTheReturn = returned->startFrame();
    outcome.stayedVerified = stayed.session()->verifiedFrame();
    outcome.returnedVerified = returned->session() == nullptr ? 0 : returned->session()->verifiedFrame();
    outcome.stayedReports = byFrame(tap.firstReports);
    outcome.droppedReports = byFrame(tap.secondReports);
    outcome.anyDesync = stayed.lastDesync().has_value() || returned->lastDesync().has_value();

    return outcome;
}

}

TEST_CASE("a player that drops and comes back with its token plays on in its slot from a snapshot")
{
    const Outcome outcome = playThroughADropAndAReturn();

    REQUIRE(outcome.slotBeforeTheDrop == 1U);
    REQUIRE(outcome.slotAfterTheReturn == outcome.slotBeforeTheDrop);
    REQUIRE(outcome.startFrameAfterTheReturn > kComesBackAt - 10U);
    REQUIRE(outcome.returnedVerified + 10U >= outcome.stayedVerified);
}

TEST_CASE("a player that drops and comes back agrees with the one that stayed on every frame both reported")
{
    const Outcome outcome = playThroughADropAndAReturn();
    std::uint32_t framesAfterTheReturn = 0;

    for (const auto& [frame, checksum] : outcome.droppedReports)
    {
        const auto stayed = outcome.stayedReports.find(frame);

        if (stayed == outcome.stayedReports.end())
        {
            continue;
        }

        CAPTURE(frame);
        REQUIRE(stayed->second == checksum);
        framesAfterTheReturn += frame > outcome.startFrameAfterTheReturn ? 1U : 0U;
    }

    REQUIRE(framesAfterTheReturn >= kHostFrames - kComesBackAt - 40U);
    REQUIRE_FALSE(outcome.anyDesync);
}
