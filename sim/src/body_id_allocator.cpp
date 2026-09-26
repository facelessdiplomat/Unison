#include <unison/sim/body_id_allocator.hpp>

#include <unison/core/contract.hpp>

namespace unison::sim
{

std::optional<BodyIdAllocator> BodyIdAllocator::fromState(std::span<const BodyId> freeIds, std::uint32_t takenSlotCount)
{
    if (takenSlotCount > kMaxBodies || freeIds.size() > takenSlotCount)
    {
        return std::nullopt;
    }

    BodyIdAllocator allocator;
    allocator.takenSlots = takenSlotCount;

    for (const BodyId id : freeIds)
    {
        if (bodyIndexOf(id) >= takenSlotCount || allocator.isFree(id))
        {
            return std::nullopt;
        }

        allocator.freeList.pushBack(id);
    }

    return allocator;
}

BodyId BodyIdAllocator::allocate()
{
    if (freeList.size() > 0)
    {
        const BodyId waiting = freeList[freeList.size() - 1];
        freeList.popBack();

        return makeBodyId(bodyIndexOf(waiting), static_cast<std::uint8_t>(bodySequenceOf(waiting) + 1U));
    }

    UNISON_VERIFY(takenSlots < kMaxBodies);

    if (takenSlots >= kMaxBodies)
    {
        return BodyId::Invalid;
    }

    const BodyId fresh = makeBodyId(takenSlots, 0U);
    ++takenSlots;

    return fresh;
}

void BodyIdAllocator::release(BodyId id)
{
    UNISON_VERIFY(bodyIndexOf(id) < takenSlots);
    UNISON_ASSERT(!isFree(id));

    if (bodyIndexOf(id) >= takenSlots)
    {
        return;
    }

    freeList.pushBack(id);
}

std::span<const BodyId> BodyIdAllocator::freeIds() const
{
    return std::span<const BodyId>{freeList.begin(), freeList.size()};
}

std::uint32_t BodyIdAllocator::takenSlotCount() const
{
    return takenSlots;
}

bool BodyIdAllocator::isFree(BodyId id) const
{
    for (const BodyId waiting : freeList)
    {
        if (bodyIndexOf(waiting) == bodyIndexOf(id))
        {
            return true;
        }
    }

    return false;
}

}
