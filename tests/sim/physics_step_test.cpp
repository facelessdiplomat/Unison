#include <unison/sim/physics_step.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/advance_frame.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

namespace
{

constexpr unison::AssetId kCrate = unison::makeAssetId("crate");
constexpr unison::AssetId kFloor = unison::makeAssetId("floor");
constexpr float kTickSeconds = 1.0F / 60.0F;

void defineArena(unison::sim::AssetRegistry& assets)
{
    unison::sim::BodyDefinition crate;
    crate.halfExtents = unison::Float3{0.5F, 0.5F, 0.5F};
    crate.motion = unison::sim::BodyMotion::Dynamic;
    crate.layer = unison::sim::PhysicsLayer::Moving;

    unison::sim::BodyDefinition floor;
    floor.halfExtents = unison::Float3{50.0F, 0.5F, 50.0F};

    assets.add<unison::sim::BodyDefinition>(kCrate, crate);
    assets.add<unison::sim::BodyDefinition>(kFloor, floor);
    assets.freeze();
}

entt::entity
spawn(unison::sim::Frame& frame, const unison::sim::AssetRegistry& assets, unison::AssetId definition, float height)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, unison::Float3{0.0F, height, 0.0F}, unison::Quaternion{});
    unison::sim::addBody(frame, assets, entity, definition);

    return entity;
}

}

TEST_CASE("a falling box comes to rest on the floor and its transform follows")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    const entt::entity floor = spawn(frame, assets, kFloor, 0.0F);
    const entt::entity crate = spawn(frame, assets, kCrate, 5.0F);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    const unison::sim::FrameInputs inputs;

    for (int tick = 0; tick < 180; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, inputs);
    }

    const unison::sim::Transform& resting = frame.registry.get<unison::sim::Transform>(crate);

    REQUIRE(resting.position.y > 0.9F);
    REQUIRE(resting.position.y < 1.1F);
    REQUIRE(frame.registry.get<unison::sim::Transform>(floor).position.y == 0.0F);
}

TEST_CASE("a box on its way down has moved by the tick after it started")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    const entt::entity crate = spawn(frame, assets, kCrate, 5.0F);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});

    REQUIRE(frame.registry.get<unison::sim::Transform>(crate).position.y < 5.0F);
}

TEST_CASE("the physics step names itself for the pipeline hash")
{
    const unison::sim::PhysicsStep physics;

    REQUIRE(physics.name() == "PhysicsStep");
}
