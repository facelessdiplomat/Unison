#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unison::net
{

/// The inputs players sent for the frames the relay has not confirmed yet, a window of frames from the next
/// one to confirm on. A frame is ready once every slot in play has sent its input for it, or overdue once a
/// deadline has passed since its first input arrived; frames are confirmed in order, each once.
class InputCollector
{
public:
    InputCollector(std::uint8_t slotCount, std::uint8_t inputSize, std::uint32_t window);

    /// Keeps a slot's input for a frame, noting when the first input for that frame arrived. An input for a
    /// frame already confirmed or beyond the window, or a second one from the same slot, is left out.
    void collect(std::uint32_t frame, std::uint8_t slot, std::span<const std::byte> input, std::uint64_t now);

    [[nodiscard]] std::uint32_t nextFrame() const;

    /// The newest frame any slot has sent an input for that was kept, which is how far the fastest slot has
    /// got; nought before the first input.
    [[nodiscard]] std::uint32_t newestFrame() const;

    /// Whether every slot in the mask, one bit per slot, has sent its input for the next frame; with no slot
    /// in play no frame is ever ready.
    [[nodiscard]] bool isNextFrameReady(std::uint8_t slotsInPlay) const;

    /// Whether the deadline has passed since the first input for the next frame arrived; a frame nobody has
    /// sent an input for is never overdue.
    [[nodiscard]] bool isNextFrameOverdue(std::uint64_t now, std::uint64_t deadline) const;

    /// Writes the next frame as a confirmation carries it, per slot a flags byte then the input: a slot in play
    /// present with its input, or dropped with the last input confirmed for it; a slot out of play absent and
    /// neutral. Then moves on to the frame after it.
    void confirmNextFrame(std::uint8_t slotsInPlay, std::span<std::byte> slots);

private:
    void confirmSlot(std::uint8_t slot, bool isSlotInPlay, std::span<std::byte> confirmed);

    [[nodiscard]] std::size_t indexOf(std::uint32_t frame, std::uint8_t slot) const;

    std::uint8_t slotCount;
    std::uint8_t inputSize;
    std::uint32_t window;
    std::uint32_t next = 1;
    std::uint32_t newest = 0;
    std::vector<std::byte> inputs;
    std::vector<std::uint8_t> received;
    std::vector<std::uint64_t> firstArrivals;
    std::vector<std::byte> lastConfirmed;
};

}
