#include <unison/net/confirmed_log.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace
{

constexpr std::size_t kFrameSize = 3;

std::array<std::byte, kFrameSize> frameOf(std::uint8_t value)
{
    return {std::byte{value}, std::byte{value}, std::byte{value}};
}

}

TEST_CASE("a new confirmed log holds no frame")
{
    const unison::net::ConfirmedLog log{kFrameSize};

    REQUIRE(log.lastFrame() == 0U);
}

TEST_CASE("a confirmed log numbers its frames from one in the order they were appended")
{
    unison::net::ConfirmedLog log{kFrameSize};

    log.append(frameOf(1));
    log.append(frameOf(2));

    REQUIRE(log.lastFrame() == 2U);
    REQUIRE(std::ranges::equal(log.slotsOf(1, 1), frameOf(1)));
    REQUIRE(std::ranges::equal(log.slotsOf(2, 1), frameOf(2)));
}

TEST_CASE("a confirmed log hands out frames in a row, one after another")
{
    unison::net::ConfirmedLog log{kFrameSize};
    log.append(frameOf(1));
    log.append(frameOf(2));
    log.append(frameOf(3));

    const std::array<std::byte, 2 * kFrameSize> secondAndThird{
        std::byte{2}, std::byte{2}, std::byte{2}, std::byte{3}, std::byte{3}, std::byte{3}};

    REQUIRE(std::ranges::equal(log.slotsOf(2, 2), secondAndThird));
}

TEST_CASE("asking a confirmed log for a frame it does not hold breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::net::ConfirmedLog log{kFrameSize};
    log.append(frameOf(1));

    static_cast<void>(log.slotsOf(1, 2));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a confirmed log holds the frames from one to its last and no other")
{
    unison::net::ConfirmedLog log{kFrameSize};
    log.append(frameOf(1));
    log.append(frameOf(2));

    REQUIRE_FALSE(log.holds(0));
    REQUIRE(log.holds(1));
    REQUIRE(log.holds(2));
    REQUIRE_FALSE(log.holds(3));
}
