#include <unison/session/input_buffer.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>

namespace unison::session
{

InputBuffer::InputBuffer(std::uint32_t capacity) : entries(capacity)
{
    UNISON_VERIFY(capacity > 0);
}

bool InputBuffer::store(
    std::uint32_t frame, std::size_t slot, std::span<const std::byte> input, sim::InputFlags flags, InputState state)
{
    if (!isInWindow(frame))
    {
        return false;
    }

    Entry& entry = entries[indexOf(frame)];
    entry.inputs.setBytes(slot, input, flags);
    entry.states.at(slot) = state;

    return true;
}

const sim::FrameInputs& InputBuffer::inputsAt(std::uint32_t frame) const
{
    return entries[indexOf(frame)].inputs;
}

InputState InputBuffer::stateAt(std::uint32_t frame, std::size_t slot) const
{
    UNISON_VERIFY(slot < sim::kMaxSlots);

    return entries[indexOf(frame)].states.at(slot);
}

std::optional<std::uint32_t> InputBuffer::lastConfirmedBefore(std::uint32_t frame, std::size_t slot) const
{
    const std::size_t framesBelow = frame > firstFrame ? std::min<std::size_t>(frame - firstFrame, entries.size()) : 0;

    for (std::size_t offset = framesBelow; offset > 0; --offset)
    {
        const auto candidate = static_cast<std::uint32_t>(firstFrame + offset - 1);

        if (stateAt(candidate, slot) == InputState::Confirmed)
        {
            return candidate;
        }
    }

    return std::nullopt;
}

void InputBuffer::evictBelow(std::uint32_t frame)
{
    UNISON_VERIFY(frame >= firstFrame);

    const std::size_t evicted = std::min<std::size_t>(frame - firstFrame, entries.size());

    for (std::size_t offset = 0; offset < evicted; ++offset)
    {
        entries[(firstFrame + offset) % entries.size()] = Entry{};
    }

    firstFrame = frame;
}

bool InputBuffer::isInWindow(std::uint32_t frame) const
{
    return frame >= firstFrame && frame - firstFrame < entries.size();
}

std::size_t InputBuffer::indexOf(std::uint32_t frame) const
{
    UNISON_VERIFY(isInWindow(frame));

    return frame % entries.size();
}

}
