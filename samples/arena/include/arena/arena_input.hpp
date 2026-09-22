#pragma once

#include <cstdint>

namespace arena
{

/// The buttons a player can hold down in one tick.
enum class Button : std::uint16_t
{
    Jump = 1U << 0U,
    Fire = 1U << 1U
};

/// What a player asks for in one tick, quantised so that no float of the host reaches the
/// simulation: movement as two axes from -127 to 127, aim as a whole turn in 65536 steps.
struct ArenaInput
{
    std::int8_t moveX = 0;
    std::int8_t moveY = 0;
    std::int16_t yaw = 0;
    std::uint16_t buttons = 0;
};

/// Whether the player is holding that button down.
[[nodiscard]] constexpr bool isHeld(const ArenaInput& input, Button button)
{
    return (input.buttons & static_cast<std::uint16_t>(button)) != 0U;
}

/// Turns the quantised axis of an input into the fraction of full speed it asks for.
[[nodiscard]] constexpr float axisOf(std::int8_t axis)
{
    return static_cast<float>(axis) / 127.0F;
}

/// Turns the quantised aim of an input into radians.
[[nodiscard]] constexpr float yawOf(const ArenaInput& input)
{
    constexpr float kTurn = 6.28318530717958647692F;

    return static_cast<float>(input.yaw) * (kTurn / 65536.0F);
}

}
