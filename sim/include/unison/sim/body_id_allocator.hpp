#pragma once

#include <unison/core/body_id.hpp>
#include <unison/core/fixed_vector.hpp>

#include <cstdint>
#include <optional>
#include <span>

namespace unison::sim
{

/// Hands out body ids and takes them back. It is frame state, so the same run of calls yields the
/// same ids on every client and after every restore, and a slot returns only with its sequence
/// number moved on, so a handle kept from before cannot name the body that took its place.
class BodyIdAllocator
{
public:
    /// An allocator that stands where one with these free ids and slots taken stood, or nothing when no allocator
    /// could: more slots than the table holds, a free id beyond the slots taken, or one slot free twice.
    [[nodiscard]] static std::optional<BodyIdAllocator> fromState(std::span<const BodyId> freeIds,
                                                                  std::uint32_t takenSlotCount);

    [[nodiscard]] BodyId allocate();

    void release(BodyId id);

    /// The ids waiting to be handed out again, the one released last at the end.
    [[nodiscard]] std::span<const BodyId> freeIds() const;

    /// How many slots have been taken from the body table at all, in use or waiting.
    [[nodiscard]] std::uint32_t takenSlotCount() const;

private:
    [[nodiscard]] bool isFree(BodyId id) const;

    FixedVector<BodyId, kMaxBodies> freeList;
    std::uint32_t takenSlots = 0;
};

}
