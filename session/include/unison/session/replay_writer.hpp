#pragma once

#include <unison/net/session_config.hpp>
#include <unison/session/replay_format.hpp>
#include <unison/session/verified_frame_receiver.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace unison::session
{

/// Writes a match as a replay: the header of its config at once, then frames and checksums in the order they
/// are given, or as a session verifies them when it is the session's receiver. A config no session can play
/// breaks a contract.
class ReplayWriter final : public IVerifiedFrameReceiver
{
public:
    explicit ReplayWriter(const net::SessionConfig& config);

    /// Writes the frame, then its checksum when the session took one.
    void frameVerified(const VerifiedFrame& frame) override;

    /// Appends a frame: its number, then every slot's flags and as many bytes of its input as the config says.
    void writeFrame(std::uint32_t frameNumber, const sim::FrameInputs& inputs);

    void writeChecksum(const VerifiedChecksum& checksum);

    /// Every byte written so far, the header first.
    [[nodiscard]] std::span<const std::byte> bytes() const;

private:
    net::SessionConfig config;
    std::vector<std::byte> encoded;
};

}
