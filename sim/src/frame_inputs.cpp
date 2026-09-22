#include <unison/sim/frame_inputs.hpp>

namespace unison::sim
{

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
