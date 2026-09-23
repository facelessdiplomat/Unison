#pragma once

#include <unison/sim/frame.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <cstdint>

namespace unison::sim
{

/// Folds a whole frame into one number: the globals member by member, the identifiers and free list
/// of the entity storage, then every component pool in registration order. Two frames agree on this
/// number when and only when they would simulate the same from here on.
[[nodiscard]] std::uint64_t checksumOf(const Frame& frame);

/// Folds a snapshot into the number its frame had when the snapshot was taken, so a frame can be
/// checksummed long after the live frame has moved on.
[[nodiscard]] std::uint64_t checksumOf(const FrameSnapshot& snapshot);

}
