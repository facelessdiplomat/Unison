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
constexpr std::uint32_t kDueFrame = 100;
constexpr std::uint64_t kSentAt = 1'000'000;
constexpr std::uint64_t kCameBackAt = kSentAt + kRoundTrip;
constexpr std::uint32_t kRoundTripFrames = kRoundTrip / kTick;

void observeAhead(unison::session::TimeSync& sync, std::int32_t framesAhead, std::uint32_t pongs)
{
    const auto predicted =
        static_cast<std::uint32_t>(static_cast<std::int32_t>(kDueFrame + kRoundTripFrames) + framesAhead);

    for (std::uint32_t pong = 0; pong < pongs; ++pong)
    {
        sync.observe(unison::net::Pong{kSentAt, 0, kDueFrame}, kSentAt + kRoundTripFrames * kTick, predicted);
    }
}

std::int32_t sumOfCorrections(unison::session::TimeSync& sync, std::uint32_t hostFrames)
{
    std::int32_t sum = 0;

    for (std::uint32_t frame = 0; frame < hostFrames; ++frame)
    {
        sum += sync.takeCorrection(kCameBackAt + frame * kTick);
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
            const auto due = static_cast<std::uint32_t>(kStartFrame + static_cast<std::int64_t>(sentAt / kTick));

            sync.observe(unison::net::Pong{sentAt, 0, due}, now, static_cast<std::uint32_t>(clientFrame));
            pingsInFlight.pop_front();
        }

        if (hostFrame % kHostFramesPerPing == 0)
        {
            pingsInFlight.push_back(now);
        }

        clientFrame += 1 + sync.takeCorrection(now);

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

TEST_CASE("a client half a round trip ahead of the frame the relay's clock has due is left alone")
{
    unison::session::TimeSync sync{kTickRate};

    observeAhead(sync, 0, kPongsPerJudgement);

    REQUIRE(sumOfCorrections(sync, 10) == 0);
}

TEST_CASE("a client ahead of where it should be runs a tick fewer for every frame it is ahead")
{
    unison::session::TimeSync sync{kTickRate};

    observeAhead(sync, 3, kPongsPerJudgement);

    REQUIRE(sync.takeCorrection(kCameBackAt) == -1);
    REQUIRE(sync.takeCorrection(kCameBackAt + kTick) == -1);
    REQUIRE(sync.takeCorrection(kCameBackAt + 2U * kTick) == -1);
    REQUIRE(sync.takeCorrection(kCameBackAt + 3U * kTick) == 0);
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

TEST_CASE("pongs from before the relay's clock started are left out")
{
    unison::session::TimeSync sync{kTickRate};

    for (std::uint32_t pong = 0; pong < kPongsPerJudgement; ++pong)
    {
        sync.observe(unison::net::Pong{kSentAt, 0, 0}, kCameBackAt, 50);
    }

    REQUIRE(sumOfCorrections(sync, 60) == 0);
}

TEST_CASE("pongs that come back while a correction runs are left out")
{
    unison::session::TimeSync sync{kTickRate};
    observeAhead(sync, 3, kPongsPerJudgement);
    static_cast<void>(sync.takeCorrection(kCameBackAt));

    observeAhead(sync, -10, kPongsPerJudgement);

    REQUIRE(sumOfCorrections(sync, 20) == -2);
}

TEST_CASE("pongs of pings sent before a correction had run its course are left out, however late they come back")
{
    unison::session::TimeSync sync{kTickRate};
    observeAhead(sync, -3, kPongsPerJudgement);
    const std::uint64_t correctedAt = kCameBackAt + 2U * kTick;
    static_cast<void>(sync.takeCorrection(kCameBackAt));
    static_cast<void>(sync.takeCorrection(kCameBackAt + kTick));
    static_cast<void>(sync.takeCorrection(correctedAt));

    for (std::uint32_t pong = 0; pong < kPongsPerJudgement; ++pong)
    {
        sync.observe(unison::net::Pong{correctedAt, 0, kDueFrame + 10}, correctedAt + kRoundTrip, kDueFrame);
    }

    REQUIRE(sumOfCorrections(sync, 20) == 0);
}

TEST_CASE("the round trip is how long the last pong took to come back")
{
    unison::session::TimeSync sync{kTickRate};

    sync.observe(unison::net::Pong{2'000'000, 10, 10}, 2'000'000 + 87'000, 16);

    REQUIRE(sync.roundTripMicroseconds() == 87'000U);
}

TEST_CASE("the lead is how far ahead of the relay's clock the client played when the last pong came back")
{
    unison::session::TimeSync sync{kTickRate};

    sync.observe(unison::net::Pong{kSentAt, 0, kDueFrame}, kCameBackAt, kDueFrame + 10U);

    REQUIRE(sync.leadMicroseconds() == 10 * static_cast<std::int64_t>(kTick) - static_cast<std::int64_t>(kOneWay));
}

TEST_CASE("a client has no lead before the relay's clock starts")
{
    unison::session::TimeSync sync{kTickRate};

    sync.observe(unison::net::Pong{kSentAt, 0, 0}, kCameBackAt, 10U);

    REQUIRE(sync.leadMicroseconds() == 0);
}

TEST_CASE("the lead follows every pong, those left out while a correction runs included")
{
    unison::session::TimeSync sync{kTickRate};
    observeAhead(sync, 10, kPongsPerJudgement);

    sync.observe(unison::net::Pong{kSentAt, 0, kDueFrame}, kCameBackAt, kDueFrame + 3U);

    REQUIRE(sync.leadMicroseconds() == 3 * static_cast<std::int64_t>(kTick) - static_cast<std::int64_t>(kOneWay));
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

TEST_CASE("frames the relay confirms late for a player who is out do not slow a client down")
{
    unison::session::TimeSync sync{kTickRate};
    constexpr std::uint32_t kConfirmedAtTheDeadline = kDueFrame - 6;

    for (std::uint32_t pong = 0; pong < kPongsPerJudgement; ++pong)
    {
        sync.observe(
            unison::net::Pong{kSentAt, kConfirmedAtTheDeadline, kDueFrame}, kCameBackAt, kDueFrame + kRoundTripFrames);
    }

    REQUIRE(sumOfCorrections(sync, 10) == 0);
}
