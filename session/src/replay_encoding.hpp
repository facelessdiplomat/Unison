#pragma once

#include <unison/net/session_config.hpp>
#include <unison/sim/frame_inputs.hpp>

#include <cstdint>

namespace unison::session
{

enum class ReplayRecordKind : std::uint8_t
{
    Frame = 1,
    Checksum = 2
};

[[nodiscard]] inline bool fitsFrameInputs(const net::SessionConfig& config)
{
    return config.slotCount <= sim::kMaxSlots && config.inputSize <= sim::kMaxInputSize;
}

}
