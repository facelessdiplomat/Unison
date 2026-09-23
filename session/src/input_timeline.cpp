#include <unison/session/input_timeline.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>

namespace unison::session
{

InputTimeline::InputTimeline(std::size_t slotCount, std::size_t localSlot, std::uint32_t capacity)
    : slotCount{slotCount}, localSlot{localSlot}, inputBuffer{capacity}, localInput{localSlot}
{
    UNISON_VERIFY(slotCount <= sim::kMaxSlots);
    UNISON_VERIFY(localSlot < slotCount);
}

void InputTimeline::setLocalInput(std::span<const std::byte> input)
{
    localInput.set(input);
}

void InputTimeline::sampleLocal(std::uint32_t frame)
{
    if (inputBuffer.stateAt(frame, localSlot) == InputState::Confirmed)
    {
        return;
    }

    const bool sampled = localInput.sampleInto(inputBuffer, frame);
    UNISON_VERIFY(sampled);
}

void InputTimeline::guessUnconfirmed(std::uint32_t frame)
{
    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        if (slot == localSlot || inputBuffer.stateAt(frame, slot) == InputState::Confirmed)
        {
            continue;
        }

        const bool guessed = predictor.predict(inputBuffer, frame, slot);
        UNISON_VERIFY(guessed);
    }
}

void InputTimeline::confirm(std::uint32_t frame, const sim::FrameInputs& confirmed)
{
    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        const bool stored =
            inputBuffer.store(frame, slot, confirmed.bytesAt(slot), confirmed.flagsAt(slot), InputState::Confirmed);
        UNISON_VERIFY(stored);

        if (!stored)
        {
            return;
        }
    }
}

bool InputTimeline::matches(std::uint32_t frame, const sim::FrameInputs& confirmed) const
{
    const sim::FrameInputs& held = inputBuffer.inputsAt(frame);

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        if (held.flagsAt(slot) != confirmed.flagsAt(slot) ||
            !std::ranges::equal(held.bytesAt(slot), confirmed.bytesAt(slot)))
        {
            return false;
        }
    }

    return true;
}

bool InputTimeline::isConfirmed(std::uint32_t frame) const
{
    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        if (inputBuffer.stateAt(frame, slot) != InputState::Confirmed)
        {
            return false;
        }
    }

    return true;
}

bool InputTimeline::holds(std::uint32_t frame) const
{
    return inputBuffer.holds(frame);
}

const sim::FrameInputs& InputTimeline::inputsAt(std::uint32_t frame) const
{
    return inputBuffer.inputsAt(frame);
}

void InputTimeline::evictBelow(std::uint32_t frame)
{
    inputBuffer.evictBelow(frame);
}

const InputBuffer& InputTimeline::buffer() const
{
    return inputBuffer;
}

}
