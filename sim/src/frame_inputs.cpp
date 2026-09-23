#include <unison/sim/frame_inputs.hpp>

#include <algorithm>

namespace unison::sim
{

void FrameInputs::setBytes(std::size_t slot, std::span<const std::byte> input, InputFlags flags)
{
    UNISON_VERIFY(slot < kMaxSlots);
    UNISON_VERIFY(input.size() <= kMaxInputSize);

    if (input.size() > kMaxInputSize)
    {
        return;
    }

    std::ranges::copy(input, slots.at(slot).begin());
    slotFlags.at(slot) = flags;
}

std::span<const std::byte, kMaxInputSize> FrameInputs::bytesAt(std::size_t slot) const
{
    UNISON_VERIFY(slot < kMaxSlots);

    return slots.at(slot);
}

InputFlags FrameInputs::flagsAt(std::size_t slot) const
{
    UNISON_VERIFY(slot < kMaxSlots);

    return slotFlags.at(slot);
}

void FrameInputs::setFlags(std::size_t slot, InputFlags flags)
{
    UNISON_VERIFY(slot < kMaxSlots);

    slotFlags.at(slot) = flags;
}

void FrameInputs::rememberInputType(entt::id_type typeId)
{
    UNISON_VERIFY(inputTypeId == 0 || inputTypeId == typeId);

    inputTypeId = typeId;
}

}
