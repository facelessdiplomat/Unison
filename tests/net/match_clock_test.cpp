#include <unison/net/match_clock.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstdint>

namespace
{

constexpr std::uint16_t kTickRate = 60;
constexpr std::uint64_t kFirstInputAt = 1'000'000;
constexpr std::uint64_t kOneSecond = 1'000'000;

}

TEST_CASE("no frame is due before the first input of the match arrives")
{
    const unison::net::MatchClock clock{kTickRate};

    REQUIRE(clock.dueFrameAt(kFirstInputAt) == 0U);
}

TEST_CASE("the first input to arrive falls due as it arrives, and one frame more every tick after it")
{
    unison::net::MatchClock clock{kTickRate};

    clock.anchor(3, kFirstInputAt);

    REQUIRE(clock.dueFrameAt(kFirstInputAt) == 3U);
    REQUIRE(clock.dueFrameAt(kFirstInputAt + kOneSecond / 2U) == 33U);
    REQUIRE(clock.dueFrameAt(kFirstInputAt + kOneSecond) == 63U);
}

TEST_CASE("the frames due follow the tick rate exactly, however long the match")
{
    unison::net::MatchClock clock{kTickRate};
    constexpr std::uint64_t kSevenHours = 7ULL * 3'600U * kOneSecond;

    clock.anchor(1, kFirstInputAt);

    REQUIRE(clock.dueFrameAt(kFirstInputAt + kSevenHours) == 1U + 7U * 3'600U * kTickRate);
}

TEST_CASE("inputs after the first leave the clock where it was")
{
    unison::net::MatchClock clock{kTickRate};
    clock.anchor(3, kFirstInputAt);

    clock.anchor(50, kFirstInputAt + kOneSecond / 10U);

    REQUIRE(clock.dueFrameAt(kFirstInputAt + kOneSecond) == 63U);
}

TEST_CASE("a time before the first input arrived has the first input's frame due")
{
    unison::net::MatchClock clock{kTickRate};
    clock.anchor(3, kFirstInputAt);

    REQUIRE(clock.dueFrameAt(kFirstInputAt - 1U) == 3U);
}

TEST_CASE("a match clock for a match that never ticks breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::net::MatchClock clock{0};

    REQUIRE(probe.failureCount() == 1U);
}
