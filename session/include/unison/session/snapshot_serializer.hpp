#pragma once

#include <unison/core/error.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unison::session
{

/// The first four bytes of a serialised snapshot, `UNSS` read as a little-endian number.
inline constexpr std::uint32_t kSnapshotMagic = 0x53534E55;

/// The version of the snapshot format this build writes, and the only one it reads.
inline constexpr std::uint16_t kSnapshotVersion = 1;

/// Writes a snapshot as bytes that read back into the same snapshot on any machine of the same build: the component
/// layout's hash, the frame's number and step, the globals, the registry and the physics state.
void serializeSnapshot(const sim::FrameSnapshot& snapshot, std::vector<std::byte>& bytes);

/// Reads bytes `serializeSnapshot` wrote into a snapshot. Bytes without the magic, of another version or component
/// layout, with globals or a registry no frame could hold, that end early or run on are refused, leaving the snapshot
/// half read; the physics state is taken as it is, for the physics world to check when the snapshot is restored.
[[nodiscard]] tl::expected<void, Error> deserializeSnapshot(std::span<const std::byte> bytes,
                                                            sim::FrameSnapshot& snapshot);

}
