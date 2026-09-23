#pragma once

#include <unison/session/input_buffer.hpp>
#include <unison/session/local_input.hpp>
#include <unison/session/repeat_last_input_predictor.hpp>
#include <unison/sim/frame_inputs.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace unison::session
{

/// The inputs every frame of a session's window is played on, slot by slot, and where each came from: the
/// relay confirmed it, the local player gave it, or it repeats the last input confirmed for its slot. The
/// local slot is never guessed, and nothing overwrites a slot the relay confirmed.
class InputTimeline
{
public:
    /// A local slot the session does not have, or more slots than a frame holds, breaks a contract.
    InputTimeline(std::size_t slotCount, std::size_t localSlot, std::uint32_t capacity);

    /// Replaces the input the local player plays in every frame sampled from now on.
    void setLocalInput(std::span<const std::byte> input);

    /// Writes the local player's input into a frame of the window, unless the relay has confirmed it.
    void sampleLocal(std::uint32_t frame);

    /// Guesses every slot of a frame of the window that is neither the local one nor confirmed.
    void guessUnconfirmed(std::uint32_t frame);

    /// Settles every slot of the session in a frame as the relay confirmed it; a frame outside the window
    /// breaks a contract.
    void confirm(std::uint32_t frame, const sim::FrameInputs& confirmed);

    /// Whether a frame holds, in every slot of the session, the bytes and flags the relay confirmed.
    [[nodiscard]] bool matches(std::uint32_t frame, const sim::FrameInputs& confirmed) const;

    /// Whether the relay has confirmed every slot of the session in a frame of the window.
    [[nodiscard]] bool isConfirmed(std::uint32_t frame) const;

    [[nodiscard]] bool holds(std::uint32_t frame) const;

    [[nodiscard]] const sim::FrameInputs& inputsAt(std::uint32_t frame) const;

    /// Forgets every frame below the given one, which becomes the first frame of the window.
    void evictBelow(std::uint32_t frame);

    [[nodiscard]] const InputBuffer& buffer() const;

private:
    std::size_t slotCount;
    std::size_t localSlot;
    InputBuffer inputBuffer;
    LocalInput localInput;
    RepeatLastInputPredictor predictor;
};

}
