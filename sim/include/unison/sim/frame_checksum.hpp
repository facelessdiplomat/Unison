#pragma once

#include <unison/sim/frame.hpp>

#include <cstdint>

namespace unison::sim
{

/// Folds a whole frame into one number: the globals member by member, the identifiers and free list
/// of the entity storage, then every component pool in registration order. Two frames agree on this
/// number when and only when they would simulate the same from here on.
[[nodiscard]] std::uint64_t checksumOf(const Frame& frame);

}
