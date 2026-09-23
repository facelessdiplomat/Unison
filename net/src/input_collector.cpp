#include <unison/net/input_collector.hpp>

#include <unison/core/contract.hpp>
#include <unison/net/protocol.hpp>

#include <algorithm>

namespace unison::net
{

namespace
{

bool isInPlay(std::uint8_t slotsInPlay, std::uint8_t slot)
{
    return ((slotsInPlay >> slot) & 1U) != 0U;
}

}

InputCollector::InputCollector(std::uint8_t slotCount, std::uint8_t inputSize, std::uint32_t window)
    : slotCount{slotCount}, inputSize{inputSize}, window{window}, inputs(std::size_t{window} * slotCount * inputSize),
      received(std::size_t{window} * slotCount)
{
    UNISON_VERIFY(window > 0);
}

void InputCollector::collect(std::uint32_t frame, std::uint8_t slot, std::span<const std::byte> input)
{
    UNISON_VERIFY(input.size() == inputSize);

    const bool isPending = frame >= next && frame - next < window;

    if (!isPending || slot >= slotCount || received[indexOf(frame, slot)] != 0U || input.size() != inputSize)
    {
        return;
    }

    received[indexOf(frame, slot)] = 1U;
    std::ranges::copy(input, inputs.begin() + static_cast<std::ptrdiff_t>(indexOf(frame, slot) * inputSize));
}

std::uint32_t InputCollector::nextFrame() const
{
    return next;
}

bool InputCollector::isNextFrameReady(std::uint8_t slotsInPlay) const
{
    if (slotsInPlay == 0U)
    {
        return false;
    }

    for (std::uint8_t slot = 0; slot < slotCount; ++slot)
    {
        if (isInPlay(slotsInPlay, slot) && received[indexOf(next, slot)] == 0U)
        {
            return false;
        }
    }

    return true;
}

void InputCollector::confirmNextFrame(std::uint8_t slotsInPlay, std::span<std::byte> slots)
{
    UNISON_VERIFY(slots.size() == std::size_t{slotCount} * (1U + inputSize));

    for (std::uint8_t slot = 0; slot < slotCount; ++slot)
    {
        const std::size_t index = indexOf(next, slot);
        const std::span<std::byte> confirmed = slots.subspan(std::size_t{slot} * (1U + inputSize), 1U + inputSize);
        const std::span<std::byte> stored = std::span{inputs}.subspan(index * inputSize, inputSize);

        confirmed[0] = static_cast<std::byte>(isInPlay(slotsInPlay, slot) ? SlotFlags::Present : SlotFlags::None);
        std::ranges::copy(stored, confirmed.begin() + 1);
        std::ranges::fill(stored, std::byte{0});
        received[index] = 0U;
    }

    ++next;
}

std::size_t InputCollector::indexOf(std::uint32_t frame, std::uint8_t slot) const
{
    return std::size_t{frame % window} * slotCount + slot;
}

}
