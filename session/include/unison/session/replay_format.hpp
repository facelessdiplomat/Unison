#pragma once

#include <unison/session/verified_checksum.hpp>
#include <unison/sim/frame_inputs.hpp>

#include <cstdint>
#include <variant>

namespace unison::session
{

/// The first four bytes of every replay, `UNRP` read as a little-endian number.
inline constexpr std::uint32_t kReplayMagic = 0x50524E55;

/// The version of the replay format this build writes, and the only one it reads.
inline constexpr std::uint16_t kReplayVersion = 1;

/// One frame of a replay: its number and the inputs the relay settled for it.
struct ReplayFrame
{
    std::uint32_t frameNumber = 0;
    sim::FrameInputs inputs;
};

/// What a replay holds after its header, record after record: frames and the checksums of verified frames.
using ReplayRecord = std::variant<ReplayFrame, VerifiedChecksum>;

}
