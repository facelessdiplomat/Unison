#pragma once

#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <cstdint>
#include <optional>

namespace unison::session
{

/// A frame a session has just verified: its number, the inputs the relay settled for it, its checksum when the config's
/// interval falls on it, and its snapshot. The references hold for the length of the call only.
struct VerifiedFrame
{
    std::uint32_t frameNumber = 0;
    const sim::FrameInputs& inputs;
    std::optional<std::uint64_t> checksum;
    const sim::FrameSnapshot& snapshot;
};

/// Hears of every frame a session verifies, in order and before the frame leaves the session's window.
class IVerifiedFrameReceiver
{
public:
    virtual ~IVerifiedFrameReceiver() = default;

    virtual void frameVerified(const VerifiedFrame& frame) = 0;

protected:
    IVerifiedFrameReceiver() = default;
    IVerifiedFrameReceiver(const IVerifiedFrameReceiver&) = default;
    IVerifiedFrameReceiver& operator=(const IVerifiedFrameReceiver&) = default;
    IVerifiedFrameReceiver(IVerifiedFrameReceiver&&) = default;
    IVerifiedFrameReceiver& operator=(IVerifiedFrameReceiver&&) = default;
};

}
