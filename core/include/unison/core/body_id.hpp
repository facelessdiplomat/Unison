#pragma once

#include <unison/core/contract.hpp>

#include <cstdint>

namespace unison
{

inline constexpr std::uint32_t kBodySequenceShift = 23U;
inline constexpr std::uint32_t kMaxBodyIndex = (1U << kBodySequenceShift) - 1U;
inline constexpr std::uint32_t kMaxBodies = 1024U;

static_assert(kMaxBodies <= kMaxBodyIndex + 1U, "every body the engine allows needs an index to name it");

/// Handle to a physics body: the slot it lives in and the sequence number that tells the body there
/// now from the one a handle kept from before remembers. The bits sit where Jolt puts them, so the
/// handle crosses into the physics world as a copy.
enum class BodyId : std::uint32_t
{
    Invalid = 0xFFFFFFFFU
};

/// Packs a slot and the round it is in into a handle.
[[nodiscard]] constexpr BodyId makeBodyId(std::uint32_t index, std::uint8_t sequence)
{
    UNISON_ASSERT(index <= kMaxBodyIndex);

    return static_cast<BodyId>((static_cast<std::uint32_t>(sequence) << kBodySequenceShift) | index);
}

/// The slot a handle names.
[[nodiscard]] constexpr std::uint32_t bodyIndexOf(BodyId id)
{
    return static_cast<std::uint32_t>(id) & kMaxBodyIndex;
}

/// The round of that slot the handle was made in.
[[nodiscard]] constexpr std::uint8_t bodySequenceOf(BodyId id)
{
    return static_cast<std::uint8_t>(static_cast<std::uint32_t>(id) >> kBodySequenceShift);
}

}
