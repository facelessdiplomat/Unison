#include <unison/session/repeat_last_input_predictor.hpp>

#include <unison/session/input_buffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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
constexpr std::size_t kSlot = 2;

void storeInput(unison::session::InputBuffer& buffer,
                std::uint32_t frame,
                const SampleInput& input,
                unison::sim::InputFlags flags,
                unison::session::InputState state)
{
    REQUIRE(buffer.store(frame, kSlot, std::as_bytes(std::span{&input, 1}), flags, state));
}

}

TEST_CASE("a guess repeats the last input confirmed for its slot")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer,
               1,
               SampleInput{1, -2, 300, 5},
               unison::sim::InputFlags::Present,
               unison::session::InputState::Confirmed);
    const unison::session::RepeatLastInputPredictor predictor;

    const bool predicted = predictor.predict(buffer, 3, kSlot);

    REQUIRE(predicted);
    REQUIRE(std::ranges::equal(buffer.inputsAt(3).bytesAt(kSlot), buffer.inputsAt(1).bytesAt(kSlot)));
    REQUIRE(buffer.stateAt(3, kSlot) == unison::session::InputState::Predicted);
}

TEST_CASE("a guess carries the flags of the input it repeats and nothing else")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(buffer, 1, SampleInput{}, unison::sim::InputFlags::Dropped, unison::session::InputState::Confirmed);
    const unison::session::RepeatLastInputPredictor predictor;

    const bool predicted = predictor.predict(buffer, 2, kSlot);

    REQUIRE(predicted);
    REQUIRE(buffer.inputsAt(2).flagsAt(kSlot) == unison::sim::InputFlags::Dropped);
}

TEST_CASE("a slot with no confirmed input is guessed neutral")
{
    unison::session::InputBuffer buffer{kCapacity};
    storeInput(
        buffer, 2, SampleInput{7, 7, 7, 7}, unison::sim::InputFlags::Present, unison::session::InputState::Predicted);
    const unison::session::RepeatLastInputPredictor predictor;

    const bool predicted = predictor.predict(buffer, 2, kSlot);

    REQUIRE(predicted);
    REQUIRE(
        std::ranges::all_of(buffer.inputsAt(2).bytesAt(kSlot), [](std::byte value) { return value == std::byte{0}; }));
    REQUIRE(buffer.inputsAt(2).flagsAt(kSlot) == unison::sim::InputFlags::None);
    REQUIRE(buffer.stateAt(2, kSlot) == unison::session::InputState::Predicted);
}

TEST_CASE("a guess for a frame outside the window is refused")
{
    unison::session::InputBuffer buffer{kCapacity};
    const unison::session::RepeatLastInputPredictor predictor;

    const bool predicted = predictor.predict(buffer, kCapacity, kSlot);

    REQUIRE_FALSE(predicted);
}
