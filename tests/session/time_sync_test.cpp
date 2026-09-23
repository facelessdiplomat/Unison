#include <unison/session/time_sync.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstdint>
#include <cstdlib>
#include <deque>

namespace
{

constexpr std::uint16_t kTickRate = 60;
constexpr std::uint64_t kTick = 1'000'000U / kTickRate;
constexpr std::uint64_t kOneWay = 50'000;
constexpr std::uint64_t kRoundTrip = 2U * kOneWay;
constexpr std::uint32_t kPongsPerJudgement = 4;

void observeAhead(unison::session::TimeSync& sync, std::int64_t framesAhead, std::uint32_t pongs)
{
    constexpr std::uint32_t kRelayFrame = 100;
    constexpr std::uint64_t kSentAt = 1'000'000;
    const auto roundTripFrames = static_cast<std::int64_t>(kRoundTrip / kTick);

    for (std::uint32_t pong = 0; pong < pongs; ++pong)
    {
        sync.observe(unison::net::Pong{kSentAt, kRelayFrame},
                     kSentAt + roundTripFrames * kTick,
                     static_cast<std::uint32_t>(kRelayFrame + roundTripFrames + framesAhead));
    }
}

std::int32_t sumOfCorrections(unison::session::TimeSync& sync, std::uint32_t hostFrames)
{
    std::int32_t sum = 0;

    for (std::uint32_t frame = 0; frame < hostFrames; ++frame)
    {
        sum += sync.takeCorrection();
    }

    return sum;
}

struct Drift
{
    std::int64_t farthestAfterSettling = 0;
    std::int64_t finalOffset = 0;
};

Drift driftOfAClientStarting(std::int64_t framesOff)
{
    constexpr std::uint64_t kHostFrames = 600;
    constexpr std::uint64_t kSettlingFrames = 240;
    constexpr std::uint64_t kHostFramesPerPing = 6;
    constexpr std::int64_t kStartFrame = 1000;

    unison::session::TimeSync sync{kTickRate};
    std::deque<std::uint64_t> pingsInFlight;
    std::int64_t clientFrame = kStartFrame + framesOff;
    Drift drift;

    for (std::uint64_t hostFrame = 0; hostFrame < kHostFrames; ++hostFrame)
    {
        const std::uint64_t now = hostFrame * kTick;

        while (!pingsInFlight.empty() && pingsInFlight.front() + kRoundTrip <= now)
        {
            const std::uint64_t sentAt = pingsInFlight.front();
            const auto relayFrame = static_cast<std::uint32_t>(kStartFrame + static_cast<std::int64_t>(sentAt / kTick));

            sync.observe(unison::net::Pong{sentAt, relayFrame}, now, static_cast<std::uint32_t>(clientFrame));
            pingsInFlight.pop_front();
        }

        if (hostFrame % kHostFramesPerPing == 0)
        {
            pingsInFlight.push_back(now);
        }

        clientFrame += 1 + sync.takeCorrection();

        const std::int64_t offset = clientFrame - kStartFrame - static_cast<std::int64_t>(hostFrame + 1U);

        if (hostFrame >= kSettlingFrames && std::llabs(offset) > std::llabs(drift.farthestAfterSettling))
        {
            drift.farthestAfterSettling = offset;
        }

        drift.finalOffset = offset;
    }

    return drift;
}

}

TEST_CASE("a client standing where it should is left alone")
{
    unison::session::TimeSync sync{kTickRate};

    observeAhead(sync, 0, kPongsPerJudgement);

    REQUIRE(sumOfCorrections(sync, 10) == 0);
}

TEST_CASE("a client ahead of where it should be runs a tick fewer for every frame it is ahead")
{
    unison::session::TimeSync sync{kTickRate};

    observeAhead(sync, 3, kPongsPerJudgement);

    REQUIRE(sync.takeCorrection() == -1);
    REQUIRE(sync.takeCorrection() == -1);
    REQUIRE(sync.takeCorrection() == -1);
    REQUIRE(sync.takeCorrection() == 0);
}

TEST_CASE("a client behind where it should be runs a tick more for every frame it is behind")
{
    unison::session::TimeSync sync{kTickRate};

    observeAhead(sync, -3, kPongsPerJudgement);

    REQUIRE(sumOfCorrections(sync, 10) == 3);
}

TEST_CASE("a drift within the jitter margin is left alone")
{
    unison::session::TimeSync sync{kTickRate};

    observeAhead(sync, 1, kPongsPerJudgement);

    REQUIRE(sumOfCorrections(sync, 10) == 0);
}

TEST_CASE("a client is judged only once enough pongs have come back")
{
    unison::session::TimeSync sync{kTickRate};

    observeAhead(sync, 5, kPongsPerJudgement - 1);

    REQUIRE(sumOfCorrections(sync, 10) == 0);
}

TEST_CASE("pongs that come back while a correction runs are left out")
{
    unison::session::TimeSync sync{kTickRate};
    observeAhead(sync, 3, kPongsPerJudgement);
    static_cast<void>(sync.takeCorrection());

    observeAhead(sync, -10, kPongsPerJudgement);

    REQUIRE(sumOfCorrections(sync, 20) == -2);
}

TEST_CASE("the round trip is how long the last pong took to come back")
{
    unison::session::TimeSync sync{kTickRate};

    sync.observe(unison::net::Pong{2'000'000, 10}, 2'000'000 + 87'000, 16);

    REQUIRE(sync.roundTripMicroseconds() == 87'000U);
}

TEST_CASE("a client that starts ahead settles within the jitter margin under a 100 ms round trip")
{
    const Drift drift = driftOfAClientStarting(20);

    REQUIRE(std::llabs(drift.farthestAfterSettling) <= 2);
    REQUIRE(std::llabs(drift.finalOffset) <= 2);
}

TEST_CASE("a client that starts behind catches up within the jitter margin under a 100 ms round trip")
{
    const Drift drift = driftOfAClientStarting(-20);

    REQUIRE(std::llabs(drift.farthestAfterSettling) <= 2);
    REQUIRE(std::llabs(drift.finalOffset) <= 2);
}

TEST_CASE("a time sync for a match that never ticks breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::session::TimeSync sync{0};

    REQUIRE(probe.failureCount() == 1U);
}
