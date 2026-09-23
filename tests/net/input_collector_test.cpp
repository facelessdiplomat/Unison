#include <unison/net/input_collector.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint8_t kSlots = 2;
constexpr std::uint8_t kInputSize = 1;
constexpr std::uint32_t kWindow = 4;
constexpr std::uint8_t kBothSlots = 0b11U;

std::array<std::byte, 1> inputOf(std::uint8_t value)
{
    return {std::byte{value}};
}

std::vector<std::byte> nextFrameOf(unison::net::InputCollector& collector)
{
    std::vector<std::byte> slots(std::size_t{kSlots} * (1U + kInputSize));
    collector.confirmNextFrame(kBothSlots, slots);

    return slots;
}

}

TEST_CASE("a frame is ready once every slot in play has sent its input for it")
{
    unison::net::InputCollector collector{kSlots, kInputSize, kWindow};
    collector.collect(1, 0, inputOf(5));
    const bool readyWithOne = collector.isNextFrameReady(kBothSlots);

    collector.collect(1, 1, inputOf(6));

    REQUIRE_FALSE(readyWithOne);
    REQUIRE(collector.isNextFrameReady(kBothSlots));
}

TEST_CASE("with no slot in play no frame is ever ready")
{
    const unison::net::InputCollector collector{kSlots, kInputSize, kWindow};

    REQUIRE_FALSE(collector.isNextFrameReady(0U));
}

TEST_CASE("an input for a frame beyond the window is left out")
{
    unison::net::InputCollector collector{kSlots, kInputSize, kWindow};

    collector.collect(1 + kWindow, 0, inputOf(5));
    collector.collect(1 + kWindow, 1, inputOf(6));
    collector.collect(1, 0, inputOf(1));
    collector.collect(1, 1, inputOf(1));

    const std::vector<std::byte> first = nextFrameOf(collector);
    REQUIRE(first == std::vector<std::byte>{std::byte{1}, std::byte{1}, std::byte{1}, std::byte{1}});
    REQUIRE_FALSE(collector.isNextFrameReady(kBothSlots));
}

TEST_CASE("the first input a slot sends for a frame is the one confirmed")
{
    unison::net::InputCollector collector{kSlots, kInputSize, kWindow};
    collector.collect(1, 0, inputOf(5));
    collector.collect(1, 1, inputOf(6));

    collector.collect(1, 0, inputOf(9));

    REQUIRE(nextFrameOf(collector) == std::vector<std::byte>{std::byte{1}, std::byte{5}, std::byte{1}, std::byte{6}});
}

TEST_CASE("a confirmed frame moves the collector on to the frame after it")
{
    unison::net::InputCollector collector{kSlots, kInputSize, kWindow};
    collector.collect(1, 0, inputOf(5));
    collector.collect(1, 1, inputOf(6));

    static_cast<void>(nextFrameOf(collector));

    REQUIRE(collector.nextFrame() == 2U);
    REQUIRE_FALSE(collector.isNextFrameReady(kBothSlots));
}

TEST_CASE("an input collector needs room for at least one frame")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::net::InputCollector collector{kSlots, kInputSize, 0};

    REQUIRE(probe.failureCount() == 1U);
}
