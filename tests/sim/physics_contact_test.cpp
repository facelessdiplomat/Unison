#include <unison/sim/physics_world.hpp>

#include <cstddef>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/body_id_allocator.hpp>
#include <unison/sim/contact.hpp>
#include <unison/sim/transform.hpp>

namespace
{

constexpr float kTickSeconds = 1.0F / 60.0F;

unison::BodyId putFloor(unison::sim::PhysicsWorld& world, unison::sim::BodyIdAllocator& ids)
{
    unison::sim::BodyDefinition floor;
    floor.halfExtents = unison::Float3{50.0F, 0.5F, 50.0F};

    const unison::BodyId id = ids.allocate();
    world.createBody(id, floor, unison::sim::Transform{});

    return id;
}

unison::BodyId putCrate(unison::sim::PhysicsWorld& world, unison::sim::BodyIdAllocator& ids, float x, float y)
{
    unison::sim::BodyDefinition crate;
    crate.halfExtents = unison::Float3{0.5F, 0.5F, 0.5F};
    crate.motion = unison::sim::BodyMotion::Dynamic;
    crate.layer = unison::sim::PhysicsLayer::Moving;

    const unison::BodyId id = ids.allocate();
    world.createBody(id, crate, unison::sim::Transform{unison::Float3{x, y, 0.0F}, unison::Quaternion{}});

    return id;
}

bool stepUntilContact(unison::sim::PhysicsWorld& world, int ticks)
{
    for (int tick = 0; tick < ticks; ++tick)
    {
        world.step(kTickSeconds);

        if (!world.contacts().empty())
        {
            return true;
        }
    }

    return false;
}

}

TEST_CASE("two bodies that meet are told about once, lower id first")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    const unison::BodyId floor = putFloor(world, ids);
    const unison::BodyId crate = putCrate(world, ids, 0.0F, 1.05F);

    REQUIRE(stepUntilContact(world, 60));

    REQUIRE(world.contacts().size() == 1U);
    REQUIRE(world.contacts()[0].first == floor);
    REQUIRE(world.contacts()[0].second == crate);
}

TEST_CASE("a contact that has just begun is told apart from one that goes on")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    putFloor(world, ids);
    putCrate(world, ids, 0.0F, 1.05F);

    REQUIRE(stepUntilContact(world, 60));
    REQUIRE(world.contacts()[0].phase == unison::sim::ContactPhase::Began);

    world.step(kTickSeconds);

    REQUIRE(world.contacts().size() == 1U);
    REQUIRE(world.contacts()[0].phase == unison::sim::ContactPhase::Continued);
}

TEST_CASE("a world where nothing touches tells of no contacts")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    putCrate(world, ids, 0.0F, 20.0F);

    world.step(kTickSeconds);

    REQUIRE(world.contacts().empty());
}

TEST_CASE("the contacts of a tick come in body order")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    putFloor(world, ids);
    putCrate(world, ids, 3.0F, 1.05F);
    putCrate(world, ids, -3.0F, 1.05F);

    REQUIRE(stepUntilContact(world, 60));
    world.step(kTickSeconds);

    REQUIRE(world.contacts().size() == 2U);
    REQUIRE(world.contacts()[0].second < world.contacts()[1].second);
}

TEST_CASE("a world put back to a saved state has no contacts to tell of")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    putFloor(world, ids);
    putCrate(world, ids, 0.0F, 1.05F);

    std::vector<std::byte> state;
    world.saveState(state);

    REQUIRE(stepUntilContact(world, 60));

    world.restoreState(state);

    REQUIRE(world.contacts().empty());
}
