#pragma once

#include <unison/sim/frame_inputs.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace unison::session
{

/// The inputs of one frame of a relay's confirmation, which carries per slot a flags byte and then the input, as a
/// frame plays them.
[[nodiscard]] sim::FrameInputs
inputsOfConfirmedFrame(std::span<const std::byte> slots, std::uint8_t slotCount, std::uint8_t inputSize);

}
