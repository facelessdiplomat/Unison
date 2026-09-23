#include <unison/net/clock.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("a manual clock stands still until it is moved on")
{
    unison::net::ManualClock clock;
    const std::uint64_t before = clock.nowMicroseconds();

    clock.advance(250);

    REQUIRE(before == 0U);
    REQUIRE(clock.nowMicroseconds() == 250U);
}
