#pragma once

#include <unison/session/input_buffer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace unison::session
{

/// The input the host last gave for the local player, played on every tick until the host gives
/// another; before the first one a tick plays the neutral input. A tick samples it as present and
/// unconfirmed, which is how the relay settles it unless it gives the input up.
class LocalInput
{
public:
    explicit LocalInput(std::size_t slot);

    /// Replaces the input the coming ticks play. More bytes than a slot holds break a contract.
    void set(std::span<const std::byte> input);

    /// Writes the input into the local slot of a frame; false when the buffer refuses the frame.
    [[nodiscard]] bool sampleInto(InputBuffer& buffer, std::uint32_t frame) const;

private:
    std::size_t localSlot;
    std::array<std::byte, sim::kMaxInputSize> latest{};
};

}
