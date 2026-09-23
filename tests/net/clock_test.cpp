#include <unison/net/clock.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <thread>

TEST_CASE("a manual clock stands still until it is moved on")
{
    unison::net::ManualClock clock;
    const std::uint64_t before = clock.nowMicroseconds();

    clock.advance(250);

    REQUIRE(before == 0U);
    REQUIRE(clock.nowMicroseconds() == 250U);
}

TEST_CASE("a steady clock counts from when it was made")
{
    const unison::net::SteadyClock clock;

    REQUIRE(clock.nowMicroseconds() < 1'000'000U);
}

TEST_CASE("a steady clock moves on as time passes")
{
    const unison::net::SteadyClock clock;
    const std::uint64_t before = clock.nowMicroseconds();

    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    REQUIRE(clock.nowMicroseconds() >= before + 5'000U);
}
