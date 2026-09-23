#pragma once

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

    /// The slots of a frame the log holds; asking for any other breaks a contract.
    [[nodiscard]] std::span<const std::byte> slotsOf(std::uint32_t frame) const;

private:
    std::size_t frameSize;
    std::vector<std::byte> frames;
};

}
