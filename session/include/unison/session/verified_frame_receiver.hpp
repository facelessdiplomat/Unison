#pragma once

#include <unison/sim/frame_inputs.hpp>

#include <cstdint>
#include <optional>

namespace unison::session
{

/// Hears of every frame a session verifies, in order and before the frame leaves the session's window.
class IVerifiedFrameReceiver
{
public:
    virtual ~IVerifiedFrameReceiver() = default;

    /// A frame verified: the inputs the relay settled for it and, when the config's interval falls on it, its
    /// checksum.
    virtual void
    frameVerified(std::uint32_t frameNumber, const sim::FrameInputs& inputs, std::optional<std::uint64_t> checksum) = 0;

protected:
    IVerifiedFrameReceiver() = default;
    IVerifiedFrameReceiver(const IVerifiedFrameReceiver&) = default;
    IVerifiedFrameReceiver& operator=(const IVerifiedFrameReceiver&) = default;
    IVerifiedFrameReceiver(IVerifiedFrameReceiver&&) = default;
    IVerifiedFrameReceiver& operator=(IVerifiedFrameReceiver&&) = default;
};

}
