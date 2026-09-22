#include <unison/sim/physics_world.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/body_id_allocator.hpp>
#include <unison/sim/transform.hpp>

#include <vector>

namespace
{

unison::sim::BodyDefinition box()
{
    unison::sim::BodyDefinition definition;
    definition.halfExtents = unison::Float3{0.5F, 0.5F, 0.5F};

    return definition;
}

unison::BodyId put(unison::sim::PhysicsWorld& world, unison::sim::BodyIdAllocator& ids, float x)
{
    const unison::BodyId id = ids.allocate();

    world.createBody(id, box(), unison::sim::Transform{unison::Float3{x, 0.0F, 0.0F}, unison::Quaternion{}});

    return id;
}

std::vector<float> fractionsOf(const std::vector<unison::sim::PhysicsHit>& hits)
{
    std::vector<float> fractions;

    for (const unison::sim::PhysicsHit& hit : hits)
    {
        fractions.push_back(hit.fraction);
    }

    return fractions;
}

}

TEST_CASE("a ray meets the bodies in its way nearest first")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    const unison::BodyId nearest = put(world, ids, 1.0F);
    const unison::BodyId middle = put(world, ids, 2.0F);
    const unison::BodyId farthest = put(world, ids, 3.0F);

    std::vector<unison::sim::PhysicsHit> hits;
    world.raycast(unison::Float3{0.0F, 0.0F, 0.0F}, unison::Float3{5.0F, 0.0F, 0.0F}, hits);

    REQUIRE(hits.size() == 3U);
    REQUIRE(hits[0].body == nearest);
    REQUIRE(hits[1].body == middle);
    REQUIRE(hits[2].body == farthest);
    REQUIRE(hits[0].fraction < hits[1].fraction);
    REQUIRE(hits[1].fraction < hits[2].fraction);
}

TEST_CASE("the order bodies were built in does not change what a ray finds")
{
    unison::sim::PhysicsWorld nearestFirst;
    unison::sim::BodyIdAllocator firstIds;

    put(nearestFirst, firstIds, 1.0F);
    put(nearestFirst, firstIds, 2.0F);
    put(nearestFirst, firstIds, 3.0F);

    unison::sim::PhysicsWorld farthestFirst;
    unison::sim::BodyIdAllocator otherIds;

    put(farthestFirst, otherIds, 3.0F);
    put(farthestFirst, otherIds, 2.0F);
    put(farthestFirst, otherIds, 1.0F);

    std::vector<unison::sim::PhysicsHit> hits;
    nearestFirst.raycast(unison::Float3{0.0F, 0.0F, 0.0F}, unison::Float3{5.0F, 0.0F, 0.0F}, hits);

    const std::vector<float> expected = fractionsOf(hits);

    farthestFirst.raycast(unison::Float3{0.0F, 0.0F, 0.0F}, unison::Float3{5.0F, 0.0F, 0.0F}, hits);

    REQUIRE(fractionsOf(hits) == expected);
}

TEST_CASE("a ray that meets nothing finds nothing")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    put(world, ids, 1.0F);

    std::vector<unison::sim::PhysicsHit> hits;
    world.raycast(unison::Float3{0.0F, 10.0F, 0.0F}, unison::Float3{5.0F, 10.0F, 0.0F}, hits);

    REQUIRE(hits.empty());
}

TEST_CASE("a sphere names the bodies it overlaps in body order")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    const unison::BodyId first = put(world, ids, 0.0F);
    const unison::BodyId second = put(world, ids, 1.2F);
    put(world, ids, 40.0F);

    std::vector<unison::BodyId> overlapped;
    world.overlapSphere(unison::Float3{0.6F, 0.0F, 0.0F}, 1.0F, overlapped);

    REQUIRE(overlapped.size() == 2U);
    REQUIRE(overlapped[0] == first);
    REQUIRE(overlapped[1] == second);
}

TEST_CASE("a capsule sweeping forward meets the nearest body first")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    const unison::BodyId nearest = put(world, ids, 2.0F);
    const unison::BodyId farthest = put(world, ids, 4.0F);

    std::vector<unison::sim::PhysicsHit> hits;
    world.sweepCapsule(unison::Float3{0.0F, 0.0F, 0.0F}, unison::Float3{6.0F, 0.0F, 0.0F}, 0.2F, 0.3F, hits);

    REQUIRE(hits.size() == 2U);
    REQUIRE(hits[0].body == nearest);
    REQUIRE(hits[1].body == farthest);
}
