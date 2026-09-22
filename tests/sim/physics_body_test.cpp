#include <unison/sim/physics_body.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/body_id_allocator.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

namespace
{

constexpr unison::AssetId kCrate = unison::makeAssetId("crate");
constexpr unison::AssetId kFloor = unison::makeAssetId("floor");
constexpr float kTickSeconds = 1.0F / 60.0F;

unison::sim::BodyDefinition crateDefinition()
{
    unison::sim::BodyDefinition definition;
    definition.halfExtents = unison::Float3{0.5F, 0.5F, 0.5F};
    definition.shape = unison::sim::BodyShape::Box;
    definition.motion = unison::sim::BodyMotion::Dynamic;
    definition.layer = unison::sim::PhysicsLayer::Moving;

    return definition;
}

unison::sim::BodyDefinition floorDefinition()
{
    unison::sim::BodyDefinition definition;
    definition.halfExtents = unison::Float3{50.0F, 0.5F, 50.0F};
    definition.shape = unison::sim::BodyShape::Box;
    definition.motion = unison::sim::BodyMotion::Static;
    definition.layer = unison::sim::PhysicsLayer::Static;

    return definition;
}

entt::entity place(unison::sim::Frame& frame, const unison::Float3& position)
{
    const entt::entity entity = frame.registry.create();
    frame.registry.emplace<unison::sim::Transform>(entity, position, unison::Quaternion{});

    return entity;
}

}

TEST_CASE("an entity given a body holds it in the world under the id it was handed")
{
    unison::sim::Frame frame;
    unison::sim::AssetRegistry assets;

    assets.add<unison::sim::BodyDefinition>(kCrate, crateDefinition());
    assets.freeze();

    const entt::entity entity = place(frame, unison::Float3{0.0F, 2.0F, 0.0F});

    unison::sim::addBody(frame, assets, entity, kCrate);

    const unison::sim::PhysicsBody& body = frame.registry.get<unison::sim::PhysicsBody>(entity);

    REQUIRE(body.id == unison::makeBodyId(0U, 0U));
    REQUIRE(body.definition == kCrate);
    REQUIRE(frame.physics.holdsBody(body.id));
    REQUIRE(frame.physics.bodyCount() == 1U);
}

TEST_CASE("a body is put where the transform of its entity says")
{
    unison::sim::Frame frame;
    unison::sim::AssetRegistry assets;

    assets.add<unison::sim::BodyDefinition>(kCrate, crateDefinition());
    assets.freeze();

    const entt::entity entity = place(frame, unison::Float3{1.0F, 2.0F, 3.0F});

    unison::sim::addBody(frame, assets, entity, kCrate);

    const unison::sim::Transform placed =
        frame.physics.transformOf(frame.registry.get<unison::sim::PhysicsBody>(entity).id);

    REQUIRE(placed.position.x == 1.0F);
    REQUIRE(placed.position.y == 2.0F);
    REQUIRE(placed.position.z == 3.0F);
}

TEST_CASE("a removed body leaves the world and gives its id back")
{
    unison::sim::Frame frame;
    unison::sim::AssetRegistry assets;

    assets.add<unison::sim::BodyDefinition>(kCrate, crateDefinition());
    assets.freeze();

    const entt::entity entity = place(frame, unison::Float3{0.0F, 2.0F, 0.0F});

    unison::sim::addBody(frame, assets, entity, kCrate);

    const unison::BodyId id = frame.registry.get<unison::sim::PhysicsBody>(entity).id;

    unison::sim::removeBody(frame, entity);

    REQUIRE_FALSE(frame.physics.holdsBody(id));
    REQUIRE(frame.physics.bodyCount() == 0U);
    REQUIRE_FALSE(frame.registry.all_of<unison::sim::PhysicsBody>(entity));
    REQUIRE(frame.globals.bodyIds.allocate() == unison::makeBodyId(unison::bodyIndexOf(id), 1U));
}

TEST_CASE("a body is built from each shape a definition can name")
{
    unison::sim::Frame frame;
    unison::sim::AssetRegistry assets;

    unison::sim::BodyDefinition sphere = crateDefinition();
    sphere.shape = unison::sim::BodyShape::Sphere;

    unison::sim::BodyDefinition capsule = crateDefinition();
    capsule.shape = unison::sim::BodyShape::Capsule;

    assets.add<unison::sim::BodyDefinition>(kCrate, crateDefinition());
    assets.add<unison::sim::BodyDefinition>(unison::makeAssetId("sphere"), sphere);
    assets.add<unison::sim::BodyDefinition>(unison::makeAssetId("capsule"), capsule);
    assets.freeze();

    unison::sim::addBody(frame, assets, place(frame, unison::Float3{0.0F, 2.0F, 0.0F}), kCrate);
    unison::sim::addBody(frame, assets, place(frame, unison::Float3{2.0F, 2.0F, 0.0F}), unison::makeAssetId("sphere"));
    unison::sim::addBody(frame, assets, place(frame, unison::Float3{4.0F, 2.0F, 0.0F}), unison::makeAssetId("capsule"));

    REQUIRE(frame.physics.bodyCount() == 3U);
}

TEST_CASE("the motion a definition asks for decides whether a body moves")
{
    unison::sim::Frame frame;
    unison::sim::AssetRegistry assets;

    assets.add<unison::sim::BodyDefinition>(kCrate, crateDefinition());
    assets.add<unison::sim::BodyDefinition>(kFloor, floorDefinition());
    assets.freeze();

    const entt::entity crate = place(frame, unison::Float3{0.0F, 4.0F, 0.0F});
    const entt::entity floor = place(frame, unison::Float3{0.0F, 0.0F, 0.0F});

    unison::sim::addBody(frame, assets, crate, kCrate);
    unison::sim::addBody(frame, assets, floor, kFloor);

    const unison::BodyId crateId = frame.registry.get<unison::sim::PhysicsBody>(crate).id;
    const unison::BodyId floorId = frame.registry.get<unison::sim::PhysicsBody>(floor).id;

    for (int tick = 0; tick < 10; ++tick)
    {
        frame.physics.step(kTickSeconds);
    }

    REQUIRE(frame.physics.transformOf(crateId).position.y < 4.0F);
    REQUIRE(frame.physics.transformOf(floorId).position.y == 0.0F);
}

TEST_CASE("a body weighs what its definition says")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    unison::sim::BodyDefinition crate;
    crate.halfExtents = unison::Float3{0.5F, 0.5F, 0.5F};
    crate.motion = unison::sim::BodyMotion::Dynamic;
    crate.layer = unison::sim::PhysicsLayer::Moving;
    crate.mass = 5.0F;

    const unison::BodyId id = ids.allocate();
    world.createBody(id, crate, unison::sim::Transform{});

    REQUIRE(world.massOf(id) > 4.99F);
    REQUIRE(world.massOf(id) < 5.01F);
}

TEST_CASE("a body left without a mass is as heavy as its shape makes it")
{
    unison::sim::PhysicsWorld world;
    unison::sim::BodyIdAllocator ids;

    unison::sim::BodyDefinition crate;
    crate.halfExtents = unison::Float3{0.5F, 0.5F, 0.5F};
    crate.motion = unison::sim::BodyMotion::Dynamic;
    crate.layer = unison::sim::PhysicsLayer::Moving;

    const unison::BodyId id = ids.allocate();
    world.createBody(id, crate, unison::sim::Transform{});

    REQUIRE(world.massOf(id) > 900.0F);
}
