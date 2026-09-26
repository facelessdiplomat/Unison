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

[[nodiscard]] inline bool isPlayable(const net::SessionConfig& config)
{
    return config.slotCount > 0 && config.slotCount <= sim::kMaxSlots && config.inputSize <= sim::kMaxInputSize &&
           config.checksumInterval > 0;
}

}
