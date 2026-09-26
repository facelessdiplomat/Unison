#pragma once

#include <unison/net/protocol.hpp>
#include <unison/net/session_config.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unison::net
{

/// Every frame the relay has confirmed, as its confirmation carries the slots, numbered from one in the
/// order they were confirmed. It is what the relay sends again and what a late joiner will catch up from.
class ConfirmedLog
{
public:
    explicit ConfirmedLog(std::size_t frameSize);

    void append(std::span<const std::byte> slots);

    /// The newest frame in the log, zero while it is empty.
    [[nodiscard]] std::uint32_t lastFrame() const;

    /// Whether the log holds a frame: one numbered from one to the newest.
    [[nodiscard]] bool holds(std::uint32_t frame) const;

    /// The slots of `frameCount` frames from `firstFrame` on, frame after frame; asking for a frame the log
    /// does not hold breaks a contract.
    [[nodiscard]] std::span<const std::byte> slotsOf(std::uint32_t firstFrame, std::uint32_t frameCount) const;

private:
    std::size_t frameSize;
    std::vector<std::byte> frames;
};

/// The confirmation of `frameCount` frames of a log from `firstFrame` on, for a match of this config; frames the log
/// does not hold break a contract.
[[nodiscard]] Confirmed confirmationOf(const ConfirmedLog& log,
                                       const SessionConfig& config,
                                       std::uint32_t firstFrame,
                                       std::uint32_t frameCount);

}
