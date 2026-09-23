#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unison::net
{

/// The inputs players sent for the frames the relay has not confirmed yet, a window of frames from the next
/// one to confirm on. A frame is ready once every slot in play has sent its input for it, and frames are
/// confirmed in order, each once.
class InputCollector
{
public:
    InputCollector(std::uint8_t slotCount, std::uint8_t inputSize, std::uint32_t window);

    /// Keeps a slot's input for a frame. An input for a frame already confirmed or beyond the window, or a
    /// second input from the same slot for a frame, is left out.
    void collect(std::uint32_t frame, std::uint8_t slot, std::span<const std::byte> input);

    [[nodiscard]] std::uint32_t nextFrame() const;

    /// Whether every slot in the mask, one bit per slot, has sent its input for the next frame; with no slot
    /// in play no frame is ever ready.
    [[nodiscard]] bool isNextFrameReady(std::uint8_t slotsInPlay) const;

    /// Writes the next frame as a confirmation carries it, per slot a flags byte then the input, a slot out
    /// of play absent and neutral, and moves on to the frame after it.
    void confirmNextFrame(std::uint8_t slotsInPlay, std::span<std::byte> slots);

private:
    [[nodiscard]] std::size_t indexOf(std::uint32_t frame, std::uint8_t slot) const;

    std::uint8_t slotCount;
    std::uint8_t inputSize;
    std::uint32_t window;
    std::uint32_t next = 1;
    std::vector<std::byte> inputs;
    std::vector<std::uint8_t> received;
};

}
