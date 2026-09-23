#pragma once

#include <unison/sim/frame_inputs.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace unison::session
{

/// What the session knows of one slot's input for one frame: nothing yet, an input it played before the
/// relay settled it, whether a guess or its own player's, or the input the relay settled on.
enum class InputState : std::uint8_t
{
    Missing,
    Predicted,
    Confirmed
};

/// The inputs of the frames a rollback may still replay, per slot and with what is known of each. It
/// holds a window of `capacity` frames that starts at the verified frame and only moves forward; a
/// frame outside the window is refused, and a frame entering it starts with every slot missing.
class InputBuffer
{
public:
    explicit InputBuffer(std::uint32_t capacity);

    /// Keeps one slot's input for a frame in place of whatever was kept there. Returns false and keeps
    /// nothing when the frame lies outside the window.
    [[nodiscard]] bool store(std::uint32_t frame,
                             std::size_t slot,
                             std::span<const std::byte> input,
                             sim::InputFlags flags,
                             InputState state);

    /// The inputs of a frame inside the window, as the systems will read them.
    [[nodiscard]] const sim::FrameInputs& inputsAt(std::uint32_t frame) const;

    [[nodiscard]] InputState stateAt(std::uint32_t frame, std::size_t slot) const;

    /// The newest frame the window holds below the given one whose input for the slot is confirmed,
    /// if the window holds any.
    [[nodiscard]] std::optional<std::uint32_t> lastConfirmedBefore(std::uint32_t frame, std::size_t slot) const;

    /// Forgets every frame below the given one, which becomes the first frame of the window.
    void evictBelow(std::uint32_t frame);

private:
    struct Entry
    {
        sim::FrameInputs inputs;
        std::array<InputState, sim::kMaxSlots> states{};
    };

    [[nodiscard]] bool isInWindow(std::uint32_t frame) const;

    [[nodiscard]] std::size_t indexOf(std::uint32_t frame) const;

    std::vector<Entry> entries;
    std::uint32_t firstFrame = 0;
};

}
