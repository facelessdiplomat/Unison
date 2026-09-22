#pragma once

#include <unison/sim/asset_registry.hpp>
#include <unison/sim/frame.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace unison::sim
{

/// A frame put aside so it can be returned to. It holds everything that decides what happens next
/// and nothing that does not: the events of a tick are gone by the time one is taken.
struct FrameSnapshot
{
    std::uint32_t frameNumber = 0;
    float dt = 0.0F;
    Globals globals{};
    entt::registry registry;
    std::vector<std::byte> physicsState;
};

/// Puts the frame aside, overwriting whatever the snapshot held before.
void takeSnapshot(const Frame& frame, FrameSnapshot& snapshot);

/// Returns the frame to the state the snapshot was taken at, discarding everything the frame has
/// done since. Taking a snapshot and restoring it leaves the frame's checksum unchanged.
void restoreSnapshot(const FrameSnapshot& snapshot, Frame& frame, const AssetRegistry& assets);

}
