#include <unison/session/local_input.hpp>

#include <unison/session/input_buffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <algorithm>
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

constexpr std::uint32_t kCapacity = 8;
constexpr std::size_t kLocalSlot = 3;

std::span<const std::byte> bytesOf(const SampleInput& input)
{
    return std::as_bytes(std::span{&input, 1});
}

}

TEST_CASE("a tick the host gave no input for plays the neutral input")
{
    unison::session::InputBuffer buffer{kCapacity};
    REQUIRE(buffer.store(1,
                         kLocalSlot,
                         bytesOf(SampleInput{7, 7, 7, 7}),
                         unison::sim::InputFlags::Present,
                         unison::session::InputState::Predicted));
    const unison::session::LocalInput localInput{kLocalSlot};

    const bool sampled = localInput.sampleInto(buffer, 1);

    REQUIRE(sampled);
    REQUIRE(std::ranges::all_of(buffer.inputsAt(1).bytesAt(kLocalSlot),
                                [](std::byte value) { return value == std::byte{0}; }));
}

TEST_CASE("a tick plays the input the host gave last")
{
    unison::session::InputBuffer buffer{kCapacity};
    unison::session::LocalInput localInput{kLocalSlot};
    localInput.set(bytesOf(SampleInput{1, -2, 300, 5}));

    const bool sampled = localInput.sampleInto(buffer, 1);

    REQUIRE(sampled);
    REQUIRE(buffer.inputsAt(1).get<SampleInput>(kLocalSlot).moveY == -2);
    REQUIRE(buffer.inputsAt(1).get<SampleInput>(kLocalSlot).yaw == 300);
}

TEST_CASE("the same input is played again while the host gives no new one")
{
    unison::session::InputBuffer buffer{kCapacity};
    unison::session::LocalInput localInput{kLocalSlot};
    localInput.set(bytesOf(SampleInput{1, 0, 0, 0}));

    const bool sampledFirst = localInput.sampleInto(buffer, 1);
    const bool sampledSecond = localInput.sampleInto(buffer, 2);

    REQUIRE(sampledFirst);
    REQUIRE(sampledSecond);
    REQUIRE(buffer.inputsAt(1).get<SampleInput>(kLocalSlot).moveX == 1);
    REQUIRE(buffer.inputsAt(2).get<SampleInput>(kLocalSlot).moveX == 1);
}

TEST_CASE("an input the host replaced is not played again")
{
    unison::session::InputBuffer buffer{kCapacity};
    unison::session::LocalInput localInput{kLocalSlot};
    localInput.set(bytesOf(SampleInput{1, 0, 0, 0}));
    REQUIRE(localInput.sampleInto(buffer, 1));

    localInput.set(bytesOf(SampleInput{2, 0, 0, 0}));
    const bool sampled = localInput.sampleInto(buffer, 2);

    REQUIRE(sampled);
    REQUIRE(buffer.inputsAt(1).get<SampleInput>(kLocalSlot).moveX == 1);
    REQUIRE(buffer.inputsAt(2).get<SampleInput>(kLocalSlot).moveX == 2);
}

TEST_CASE("a local input is played as present and waits for the relay to confirm it")
{
    unison::session::InputBuffer buffer{kCapacity};
    const unison::session::LocalInput localInput{kLocalSlot};

    const bool sampled = localInput.sampleInto(buffer, 1);

    REQUIRE(sampled);
    REQUIRE(buffer.inputsAt(1).flagsAt(kLocalSlot) == unison::sim::InputFlags::Present);
    REQUIRE(buffer.stateAt(1, kLocalSlot) == unison::session::InputState::Predicted);
}

TEST_CASE("a local input for a frame outside the window is refused")
{
    unison::session::InputBuffer buffer{kCapacity};
    const unison::session::LocalInput localInput{kLocalSlot};

    const bool sampled = localInput.sampleInto(buffer, kCapacity);

    REQUIRE_FALSE(sampled);
}

TEST_CASE("a local input that does not fit a slot breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::session::LocalInput localInput{kLocalSlot};
    const std::array<std::byte, unison::sim::kMaxInputSize + 1> oversized{};

    localInput.set(oversized);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a local slot the session does not have breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::session::LocalInput localInput{unison::sim::kMaxSlots};

    REQUIRE(probe.failureCount() == 1U);
}
