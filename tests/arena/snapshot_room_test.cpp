#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_rollbacks.hpp>
#include <support/arena_script.hpp>
#include <unison/session/snapshot_ring.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

constexpr std::uint32_t kFramesPerPass = 600;
constexpr std::uint32_t kRingCapacity = 12;
constexpr std::uint32_t kRollbackInterval = 10;
constexpr std::uint32_t kRollbackDepth = 6;

std::size_t roomOf(const entt::registry& registry)
{
    std::size_t room = registry.storage<entt::entity>()->capacity();

    for (const auto& pool : registry.storage())
    {
        room += pool.second.capacity();
    }

    return room;
}

std::size_t roomOf(const arena::ArenaSimulation& match, const unison::session::SnapshotRing& ring)
{
    const std::uint32_t newest = match.frame().frameNumber;
    std::size_t room = roomOf(match.frame().registry);

    for (std::uint32_t frame = newest + 1 - kRingCapacity; frame <= newest; ++frame)
    {
        const unison::sim::FrameSnapshot& snapshot = ring.snapshotAt(frame);

        room += roomOf(snapshot.registry) + snapshot.physicsState.capacity();
    }

    return room;
}

void playPass(arena::ArenaSimulation& match, unison::session::SnapshotRing& ring, std::size_t& room)
{
    while (match.frame().frameNumber < kRingCapacity)
    {
        unison::test::playOn(match, ring);
    }

    while (match.frame().frameNumber < kFramesPerPass)
    {
        unison::test::playOn(match, ring);

        if (match.frame().frameNumber % kRollbackInterval == 0)
        {
            unison::test::rollBack(match, ring, kRollbackDepth);
        }

        const std::size_t roomNow = roomOf(match, ring);

        if (roomNow < room)
        {
            FAIL("the buffers gave back room at frame " << match.frame().frameNumber);
        }

        room = roomNow;
    }
}

}

TEST_CASE("a match rolled back through a ring of snapshots needs no more room the second time through")
{
    arena::ArenaSimulation match{unison::test::kScriptedPlayers};
    unison::session::SnapshotRing ring{kRingCapacity};
    unison::sim::FrameSnapshot start;
    unison::sim::takeSnapshot(match.frame(), start);
    ring.store(match.frame());

    std::size_t room = 0;
    playPass(match, ring, room);
    const std::size_t roomAfterFirstPass = room;

    unison::sim::restoreSnapshot(start, match.frame());
    playPass(match, ring, room);

    REQUIRE(roomAfterFirstPass > 0U);
    REQUIRE(room == roomAfterFirstPass);
}
