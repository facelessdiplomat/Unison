#pragma once

#include <arena/arena_simulation.hpp>

#include <support/arena_script.hpp>
#include <unison/session/snapshot_ring.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <cstdint>

namespace unison::test
{

/// Plays the scripted match on by one frame and keeps its snapshot in the ring.
inline void playOn(arena::ArenaSimulation& match, session::SnapshotRing& ring)
{
    match.advance(scriptedInputs(match.frame().frameNumber));
    ring.store(match.frame());
}

/// Takes the scripted match back the given number of frames through the ring and plays it on again to the
/// frame it had reached, as a rollback does.
inline void rollBack(arena::ArenaSimulation& match, session::SnapshotRing& ring, std::uint32_t depth)
{
    const std::uint32_t reached = match.frame().frameNumber;

    sim::restoreSnapshot(ring.snapshotAt(reached - depth), match.frame());

    while (match.frame().frameNumber < reached)
    {
        playOn(match, ring);
    }
}

}
