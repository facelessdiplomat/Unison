#pragma once

#include <arena/arena_input.hpp>

#include <unison/sim/frame_inputs.hpp>

#include <cstddef>
#include <cstdint>

namespace unison::test
{

/// How many players the scripted match is played with.
inline constexpr std::uint32_t kScriptedPlayers = 4;

/// The inputs every player asks for on that frame of the scripted match: a march that changes
/// direction every so often, a turn that never repeats, and jumping and firing on beats of their
/// own. It is whole-number arithmetic on the frame number, so two machines script the same match.
[[nodiscard]] inline unison::sim::FrameInputs scriptedInputs(std::uint32_t frameNumber)
{
    unison::sim::FrameInputs inputs;

    for (std::uint32_t slot = 0; slot < kScriptedPlayers; ++slot)
    {
        const std::uint32_t beat = frameNumber / 24U + slot;

        arena::ArenaInput input;
        input.moveY = static_cast<std::int8_t>((static_cast<std::int32_t>(beat % 3U) - 1) * 127);
        input.moveX = static_cast<std::int8_t>((static_cast<std::int32_t>((beat / 3U) % 3U) - 1) * 127);
        input.yaw =
            static_cast<std::int16_t>(static_cast<std::int32_t>((frameNumber * (slot + 1U) * 211U) % 65536U) - 32768);

        if ((frameNumber + slot * 7U) % 31U == 0U)
        {
            input.buttons |= static_cast<std::uint16_t>(arena::Button::Jump);
        }

        if ((frameNumber + slot * 5U) % 9U == 0U)
        {
            input.buttons |= static_cast<std::uint16_t>(arena::Button::Fire);
        }

        inputs.set(static_cast<std::size_t>(slot), input, unison::sim::InputFlags::Present);
    }

    return inputs;
}

}
