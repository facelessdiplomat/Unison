#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/allocation_probe.hpp>
#include <support/arena_rollbacks.hpp>
#include <support/arena_script.hpp>
#include <unison/session/snapshot_ring.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <cstdint>

namespace
{

constexpr std::uint32_t kFramesPerPass = 800;
constexpr std::uint32_t kRingCapacity = 12;
constexpr std::uint32_t kRollbackInterval = 10;
constexpr std::uint32_t kRollbackDepth = 6;

void playPassWithRollbacks(arena::ArenaSimulation& match, unison::session::SnapshotRing& ring)
{
    while (match.frame().frameNumber < kFramesPerPass)
    {
        unison::test::playOn(match, ring);

        if (match.frame().frameNumber >= kRingCapacity && match.frame().frameNumber % kRollbackInterval == 0)
        {
            unison::test::rollBack(match, ring, kRollbackDepth);
        }
    }
}

}

TEST_CASE("a match played through its rollbacks a second time takes nothing from the C++ heap")
{
    arena::ArenaSimulation match{unison::test::kScriptedPlayers};
    unison::session::SnapshotRing ring{kRingCapacity};
    unison::sim::FrameSnapshot start;
    unison::sim::takeSnapshot(match.frame(), start);
    ring.store(match.frame());
    playPassWithRollbacks(match, ring);
    unison::sim::restoreSnapshot(start, match.frame());

    const unison::test::AllocationProbe probe;
    playPassWithRollbacks(match, ring);

    REQUIRE(probe.cppAllocations() == 0U);
}
