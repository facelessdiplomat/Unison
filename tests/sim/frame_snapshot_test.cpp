#include <unison/sim/frame_snapshot.hpp>

#include <support/test_components.hpp>
#include <unison/sim/frame_checksum.hpp>

#include <catch2/catch_test_macros.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace
{

entt::entity spawn(unison::sim::Frame& frame, float x, std::int32_t points)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::test::Position>(entity, x, 0.0F);
    frame.registry.emplace<unison::test::Health>(entity, points);

    return entity;
}

void populate(unison::sim::Frame& frame)
{
    frame.frameNumber = 42;
    frame.dt = 1.0F / 60.0F;
    frame.globals.matchPhase = unison::sim::MatchPhase::Playing;

    const entt::entity first = spawn(frame, 1.0F, 10);
    spawn(frame, 2.0F, 20);
    spawn(frame, 3.0F, 30);
    frame.registry.destroy(first);
}

}

TEST_CASE("restoring a snapshot brings back the checksum it was taken at")
{
    unison::sim::Frame frame;
    populate(frame);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    const std::uint64_t original = unison::sim::checksumOf(frame);

    spawn(frame, 9.0F, 99);
    frame.globals.matchPhase = unison::sim::MatchPhase::Ended;
    static_cast<void>(frame.globals.rng.nextUint32());

    REQUIRE(unison::sim::checksumOf(frame) != original);

    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(unison::sim::checksumOf(frame) == original);
}

TEST_CASE("a snapshot taken again into the same holder keeps the room its pools had")
{
    unison::sim::Frame frame;
    populate(frame);
    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);
    const std::size_t room = snapshot.registry.storage<unison::test::Health>().capacity();
    frame.registry.clear<unison::test::Health>();

    unison::sim::takeSnapshot(frame, snapshot);

    REQUIRE(room > 0U);
    REQUIRE(snapshot.registry.storage<unison::test::Health>().capacity() == room);
}

TEST_CASE("a restored frame keeps the room its pools had")
{
    unison::sim::Frame frame;
    unison::sim::FrameSnapshot bare;
    unison::sim::takeSnapshot(frame, bare);
    populate(frame);
    const std::size_t room = frame.registry.storage<unison::test::Health>().capacity();

    unison::sim::restoreSnapshot(bare, frame);

    const auto* pool = std::as_const(frame.registry).storage<unison::test::Health>();

    REQUIRE(room > 0U);
    REQUIRE(pool != nullptr);
    REQUIRE(pool->capacity() == room);
}

TEST_CASE("restoring a snapshot brings back the frame number and step")
{
    unison::sim::Frame frame;
    populate(frame);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    frame.frameNumber = 100;
    frame.dt = 0.5F;

    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(frame.frameNumber == 42U);
    REQUIRE(frame.dt == 1.0F / 60.0F);
}

TEST_CASE("a restored frame hands out the identifiers its source would have")
{
    unison::sim::Frame frame;
    populate(frame);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    std::vector<entt::entity> expected;

    for (int step = 0; step < 10; ++step)
    {
        expected.push_back(frame.registry.create());
    }

    unison::sim::restoreSnapshot(snapshot, frame);

    std::vector<entt::entity> afterRestore;

    for (int step = 0; step < 10; ++step)
    {
        afterRestore.push_back(frame.registry.create());
    }

    REQUIRE(afterRestore == expected);
}

TEST_CASE("a snapshot is untouched by what happens to its frame afterwards")
{
    unison::sim::Frame frame;
    populate(frame);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    const std::size_t living = snapshot.registry.view<unison::test::Position>().size();

    frame.registry.clear();

    REQUIRE(snapshot.registry.view<unison::test::Position>().size() == living);
}

TEST_CASE("a snapshot can be taken again into the same holder")
{
    unison::sim::Frame frame;
    populate(frame);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    spawn(frame, 9.0F, 99);
    const std::uint64_t later = unison::sim::checksumOf(frame);

    unison::sim::takeSnapshot(frame, snapshot);
    unison::sim::Frame restored;
    unison::sim::restoreSnapshot(snapshot, restored);

    REQUIRE(unison::sim::checksumOf(restored) == later);
}

TEST_CASE("a restored frame hands out the body ids its source would have")
{
    unison::sim::Frame frame;
    populate(frame);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    const unison::BodyId expected = frame.globals.bodyIds.allocate();

    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(frame.globals.bodyIds.allocate() == expected);
}
