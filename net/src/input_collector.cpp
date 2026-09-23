#include <unison/net/input_collector.hpp>

#include <unison/core/contract.hpp>
#include <unison/net/protocol.hpp>

#include <algorithm>
#include <limits>

namespace unison::net
{

namespace
{

constexpr std::uint64_t kNothingArrived = std::numeric_limits<std::uint64_t>::max();

bool isInPlay(std::uint8_t slotsInPlay, std::uint8_t slot)
{
    return ((slotsInPlay >> slot) & 1U) != 0U;
}

}

InputCollector::InputCollector(std::uint8_t slotCount, std::uint8_t inputSize, std::uint32_t window)
    : slotCount{slotCount}, inputSize{inputSize}, window{window}, inputs(std::size_t{window} * slotCount * inputSize),
      received(std::size_t{window} * slotCount), firstArrivals(window, kNothingArrived),
      lastConfirmed(std::size_t{slotCount} * inputSize)
{
    UNISON_VERIFY(window > 0);
}

void InputCollector::collect(std::uint32_t frame,
                             std::uint8_t slot,
                             std::span<const std::byte> input,
                             std::uint64_t now)
{
    UNISON_VERIFY(input.size() == inputSize);

    const bool isPending = frame >= next && frame - next < window;

    if (!isPending || slot >= slotCount || received[indexOf(frame, slot)] != 0U || input.size() != inputSize)
    {
        return;
    }

    received[indexOf(frame, slot)] = 1U;
    std::ranges::copy(input, inputs.begin() + static_cast<std::ptrdiff_t>(indexOf(frame, slot) * inputSize));

    std::uint64_t& firstArrival = firstArrivals[frame % window];
    firstArrival = std::min(firstArrival, now);
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

bool InputCollector::isNextFrameOverdue(std::uint64_t now, std::uint64_t deadline) const
{
    const std::uint64_t firstArrival = firstArrivals[next % window];

    return firstArrival != kNothingArrived && now - firstArrival >= deadline;
}

void InputCollector::confirmNextFrame(std::uint8_t slotsInPlay, std::span<std::byte> slots)
{
    UNISON_VERIFY(slots.size() == std::size_t{slotCount} * (1U + inputSize));

    for (std::uint8_t slot = 0; slot < slotCount; ++slot)
    {
        confirmSlot(
            slot, isInPlay(slotsInPlay, slot), slots.subspan(std::size_t{slot} * (1U + inputSize), 1U + inputSize));
    }

    firstArrivals[next % window] = kNothingArrived;
    ++next;
}

void InputCollector::confirmSlot(std::uint8_t slot, bool isSlotInPlay, std::span<std::byte> confirmed)
{
    const std::size_t index = indexOf(next, slot);
    const std::span<std::byte> stored = std::span{inputs}.subspan(index * inputSize, inputSize);
    const std::span<std::byte> last = std::span{lastConfirmed}.subspan(std::size_t{slot} * inputSize, inputSize);
    const std::span<std::byte> input = confirmed.subspan(1);

    if (!isSlotInPlay)
    {
        confirmed[0] = static_cast<std::byte>(SlotFlags::None);
        std::ranges::fill(input, std::byte{0});
    }
    else if (received[index] != 0U)
    {
        confirmed[0] = static_cast<std::byte>(SlotFlags::Present);
        std::ranges::copy(stored, input.begin());
        std::ranges::copy(stored, last.begin());
    }
    else
    {
        confirmed[0] = static_cast<std::byte>(SlotFlags::Dropped);
        std::ranges::copy(last, input.begin());
    }

    std::ranges::fill(stored, std::byte{0});
    received[index] = 0U;
}

std::size_t InputCollector::indexOf(std::uint32_t frame, std::uint8_t slot) const
{
    return std::size_t{frame % window} * slotCount + slot;
}

}
