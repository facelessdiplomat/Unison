#include <unison/session/input_buffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
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

constexpr std::uint32_t kCapacity = 4;
constexpr std::size_t kSlot = 5;

std::span<const std::byte> bytesOf(const SampleInput& input)
{
    return std::as_bytes(std::span{&input, 1});
}

void storeInput(unison::session::InputBuffer& buffer,
                std::uint32_t frame,
                std::size_t slot,
                const SampleInput& input,
                unison::session::InputState state)
{
    REQUIRE(buffer.store(frame, slot, bytesOf(input), unison::sim::InputFlags::Present, state));
}

}

TEST_CASE("an input buffer hands back the input stored for a frame and slot")
{
    unison::session::InputBuffer buffer{kCapacity};

    const bool accepted = buffer.store(2,
                                       kSlot,
                                       bytesOf(SampleInput{1, -2, 300, 5}),
                                       unison::sim::InputFlags::Present,
                                       unison::session::InputState::Confirmed);

    const unison::sim::FrameInputs& inputs = buffer.inputsAt(2);

    REQUIRE(accepted);
    REQUIRE(inputs.get<SampleInput>(kSlot).moveX == 1);
    REQUIRE(inputs.get<SampleInput>(kSlot).moveY == -2);
    REQUIRE(inputs.get<SampleInput>(kSlot).yaw == 300);
    REQUIRE(inputs.get<SampleInput>(kSlot).buttons == 5U);
    REQUIRE(inputs.flagsAt(kSlot) == unison::sim::InputFlags::Present);
}

TEST_CASE("a stored input keeps whether it was predicted or confirmed")
{
    unison::session::InputBuffer buffer{kCapacity};

    storeInput(buffer, 1, 0, SampleInput{}, unison::session::InputState::Predicted);
    storeInput(buffer, 1, 1, SampleInput{}, unison::session::InputState::Confirmed);

    REQUIRE(buffer.stateAt(1, 0) == unison::session::InputState::Predicted);
    REQUIRE(buffer.stateAt(1, 1) == unison::session::InputState::Confirmed);
}

TEST_CASE("a slot nothing was stored for is missing")
{
    const unison::session::InputBuffer buffer{kCapacity};

    REQUIRE(buffer.stateAt(1, kSlot) == unison::session::InputState::Missing);
    REQUIRE(buffer.inputsAt(1).flagsAt(kSlot) == unison::sim::InputFlags::None);
}

TEST_CASE("frames keep their inputs apart")
{
    unison::session::InputBuffer buffer{kCapacity};

    storeInput(buffer, 1, 0, SampleInput{1, 0, 0, 0}, unison::session::InputState::Confirmed);
    storeInput(buffer, 2, 0, SampleInput{2, 0, 0, 0}, unison::session::InputState::Confirmed);

    REQUIRE(buffer.inputsAt(1).get<SampleInput>(0).moveX == 1);
    REQUIRE(buffer.inputsAt(2).get<SampleInput>(0).moveX == 2);
}

TEST_CASE("an input buffer refuses a frame beyond the end of its window")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer, 0, 0, SampleInput{1, 0, 0, 0}, unison::session::InputState::Confirmed);

    const bool accepted = buffer.store(kCapacity,
                                       0,
                                       bytesOf(SampleInput{2, 0, 0, 0}),
                                       unison::sim::InputFlags::Present,
                                       unison::session::InputState::Predicted);

    REQUIRE_FALSE(accepted);
    REQUIRE(buffer.inputsAt(0).get<SampleInput>(0).moveX == 1);
    REQUIRE(buffer.stateAt(0, 0) == unison::session::InputState::Confirmed);
}

TEST_CASE("an input buffer refuses a frame below the verified one")
{
    unison::session::InputBuffer buffer{kCapacity};
    buffer.evictBelow(10);

    const bool accepted = buffer.store(9,
                                       0,
                                       bytesOf(SampleInput{9, 0, 0, 0}),
                                       unison::sim::InputFlags::Present,
                                       unison::session::InputState::Confirmed);

    REQUIRE_FALSE(accepted);
    REQUIRE(buffer.stateAt(9 + kCapacity, 0) == unison::session::InputState::Missing);
}

TEST_CASE("evicting keeps the verified frame and every frame after it")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer, 2, 0, SampleInput{2, 0, 0, 0}, unison::session::InputState::Confirmed);
    storeInput(buffer, 3, 0, SampleInput{3, 0, 0, 0}, unison::session::InputState::Predicted);

    buffer.evictBelow(2);

    REQUIRE(buffer.inputsAt(2).get<SampleInput>(0).moveX == 2);
    REQUIRE(buffer.inputsAt(3).get<SampleInput>(0).moveX == 3);
    REQUIRE(buffer.stateAt(2, 0) == unison::session::InputState::Confirmed);
    REQUIRE(buffer.stateAt(3, 0) == unison::session::InputState::Predicted);
}

TEST_CASE("a frame entering the window finds nothing of the frame it replaces")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer, 1, 0, SampleInput{1, 0, 0, 0}, unison::session::InputState::Confirmed);

    buffer.evictBelow(2);

    REQUIRE(buffer.stateAt(1 + kCapacity, 0) == unison::session::InputState::Missing);
    REQUIRE(buffer.inputsAt(1 + kCapacity).flagsAt(0) == unison::sim::InputFlags::None);
    REQUIRE(buffer.inputsAt(1 + kCapacity).get<SampleInput>(0).moveX == 0);
}

TEST_CASE("a window moved on by more than its length holds nothing")
{
    unison::session::InputBuffer buffer{kCapacity};

    for (std::uint32_t frame = 0; frame < kCapacity; ++frame)
    {
        storeInput(buffer, frame, 0, SampleInput{1, 0, 0, 0}, unison::session::InputState::Confirmed);
    }

    buffer.evictBelow(100);

    for (std::uint32_t frame = 100; frame < 100 + kCapacity; ++frame)
    {
        REQUIRE(buffer.stateAt(frame, 0) == unison::session::InputState::Missing);
    }
}

TEST_CASE("the last confirmed frame before a frame is the newest one the window holds for the slot")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer, 1, 0, SampleInput{}, unison::session::InputState::Confirmed);
    storeInput(buffer, 2, 0, SampleInput{}, unison::session::InputState::Confirmed);
    storeInput(buffer, 3, 0, SampleInput{}, unison::session::InputState::Predicted);

    const std::optional<std::uint32_t> lastConfirmed = buffer.lastConfirmedBefore(kCapacity, 0);

    REQUIRE(lastConfirmed == std::optional<std::uint32_t>{2});
}

TEST_CASE("an input confirmed after a frame is not the last one before it")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer, 1, 0, SampleInput{}, unison::session::InputState::Confirmed);
    storeInput(buffer, 3, 0, SampleInput{}, unison::session::InputState::Confirmed);

    const std::optional<std::uint32_t> lastConfirmed = buffer.lastConfirmedBefore(2, 0);

    REQUIRE(lastConfirmed == std::optional<std::uint32_t>{1});
}

TEST_CASE("a slot the window holds no confirmed input for has no last confirmed frame")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer, 1, 0, SampleInput{}, unison::session::InputState::Predicted);
    storeInput(buffer, 1, 1, SampleInput{}, unison::session::InputState::Confirmed);

    const std::optional<std::uint32_t> lastConfirmed = buffer.lastConfirmedBefore(2, 0);

    REQUIRE_FALSE(lastConfirmed.has_value());
}

TEST_CASE("the last confirmed frame before one beyond the window is looked for inside it")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer, 2, 0, SampleInput{}, unison::session::InputState::Confirmed);

    const std::optional<std::uint32_t> lastConfirmed = buffer.lastConfirmedBefore(1000, 0);

    REQUIRE(lastConfirmed == std::optional<std::uint32_t>{2});
}

TEST_CASE("reading a frame outside the window breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::session::InputBuffer buffer{kCapacity};
    buffer.evictBelow(2);

    static_cast<void>(buffer.inputsAt(1));
    static_cast<void>(buffer.stateAt(2 + kCapacity, 0));

    REQUIRE(probe.failureCount() == 2U);
}

TEST_CASE("moving the window back breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::session::InputBuffer buffer{kCapacity};
    buffer.evictBelow(10);

    buffer.evictBelow(9);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("an input buffer needs room for at least one frame")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::session::InputBuffer buffer{0};

    REQUIRE(probe.failureCount() == 1U);
}
