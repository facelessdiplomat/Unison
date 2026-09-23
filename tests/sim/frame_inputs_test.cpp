#include <unison/sim/frame_inputs.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace
{

struct SampleInput
{
    std::int8_t moveX = 0;
    std::int8_t moveY = 0;
    std::int16_t yaw = 0;
    std::uint32_t buttons = 0;
};

struct OversizedInput
{
    std::array<std::uint64_t, 9> payload{};
};

struct PaddedInput
{
    std::uint8_t small = 0;
    std::uint32_t large = 0;
};

constexpr std::size_t kSlot = 3;

}

TEST_CASE("a plain quantised input satisfies the input traits")
{
    STATIC_REQUIRE(unison::sim::InputTraits<SampleInput>::isValid);
}

TEST_CASE("an input larger than a slot fails the traits")
{
    STATIC_REQUIRE_FALSE(unison::sim::InputTraits<OversizedInput>::isValid);
}

TEST_CASE("an input holding padding fails the traits")
{
    STATIC_REQUIRE_FALSE(unison::sim::InputTraits<PaddedInput>::isValid);
}

TEST_CASE("a slot hands back the input it was given")
{
    unison::sim::FrameInputs inputs;

    inputs.set(kSlot, SampleInput{1, -2, 300, 5}, unison::sim::InputFlags::Present);

    const SampleInput restored = inputs.get<SampleInput>(kSlot);

    REQUIRE(restored.moveX == 1);
    REQUIRE(restored.moveY == -2);
    REQUIRE(restored.yaw == 300);
    REQUIRE(restored.buttons == 5U);
}

TEST_CASE("a slot hands back the flags it was given")
{
    unison::sim::FrameInputs inputs;

    inputs.set(kSlot, SampleInput{}, unison::sim::InputFlags::Present | unison::sim::InputFlags::Predicted);

    REQUIRE(unison::sim::hasFlag(inputs.flagsAt(kSlot), unison::sim::InputFlags::Present));
    REQUIRE(unison::sim::hasFlag(inputs.flagsAt(kSlot), unison::sim::InputFlags::Predicted));
    REQUIRE_FALSE(unison::sim::hasFlag(inputs.flagsAt(kSlot), unison::sim::InputFlags::Dropped));
}

TEST_CASE("an untouched slot is absent and neutral")
{
    const unison::sim::FrameInputs inputs;

    REQUIRE(inputs.flagsAt(kSlot) == unison::sim::InputFlags::None);
    REQUIRE(inputs.get<SampleInput>(kSlot).buttons == 0U);
}

TEST_CASE("slots keep their inputs apart")
{
    unison::sim::FrameInputs inputs;

    inputs.set(0, SampleInput{1, 0, 0, 0}, unison::sim::InputFlags::Present);
    inputs.set(1, SampleInput{2, 0, 0, 0}, unison::sim::InputFlags::Present);

    REQUIRE(inputs.get<SampleInput>(0).moveX == 1);
    REQUIRE(inputs.get<SampleInput>(1).moveX == 2);
}

TEST_CASE("a slot written from bytes reads back as the input they were taken from")
{
    unison::sim::FrameInputs inputs;
    const SampleInput input{1, -2, 300, 5};

    inputs.setBytes(kSlot, std::as_bytes(std::span{&input, 1}), unison::sim::InputFlags::Present);

    REQUIRE(inputs.get<SampleInput>(kSlot).moveY == -2);
    REQUIRE(inputs.get<SampleInput>(kSlot).yaw == 300);
    REQUIRE(inputs.flagsAt(kSlot) == unison::sim::InputFlags::Present);
}

TEST_CASE("bytes that do not fit a slot break a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::FrameInputs inputs;
    const std::array<std::byte, unison::sim::kMaxInputSize + 1> oversized{};

    inputs.setBytes(0, oversized, unison::sim::InputFlags::Present);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("reading a slot as the wrong input type breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::FrameInputs inputs;

    inputs.set(0, SampleInput{}, unison::sim::InputFlags::Present);
    static_cast<void>(inputs.get<std::uint32_t>(0));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("frame inputs travel as plain bytes")
{
    STATIC_REQUIRE(std::is_trivially_copyable_v<unison::sim::FrameInputs>);
}
