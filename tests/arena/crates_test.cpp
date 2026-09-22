#include <arena/assets.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/character_move.hpp>
#include <arena/components.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/character_lifecycle.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/physics_step.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstdint>

namespace
{

constexpr float kTickSeconds = 1.0F / 60.0F;

entt::entity place(unison::sim::Frame& frame,
                   const unison::sim::AssetRegistry& assets,
                   unison::AssetId definition,
                   const unison::Float3& at)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, at, unison::Quaternion{});
    unison::sim::addBody(frame, assets, entity, definition);

    return entity;
}

entt::entity standPlayer(unison::sim::Frame& frame, const arena::PlayerStats& stats, const unison::Float3& at)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, at, unison::Quaternion{});
    frame.registry.emplace<arena::PlayerSlot>(entity, std::uint8_t{0});
    frame.registry.emplace<arena::CharacterState>(entity);

    unison::sim::CharacterController capsule;
    capsule.radius = stats.capsuleRadius;
    capsule.halfHeight = stats.capsuleHalfHeight;

    unison::sim::addCharacter(frame, entity, capsule);

    return entity;
}

}

TEST_CASE("a player walking into a crate pushes it along")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::CharacterMove move{stats};
    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(move);
    pipeline.add(physics);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    place(frame, assets, arena::kFloor, unison::Float3{0.0F, 0.0F, 0.0F});

    const entt::entity crate = place(frame, assets, arena::kCrate, unison::Float3{1.5F, 1.0F, 0.0F});
    const entt::entity player = standPlayer(frame, stats, unison::Float3{0.0F, 0.5F, 0.0F});

    const float stood = frame.registry.get<unison::sim::Transform>(crate).position.x;

    for (int tick = 0; tick < 120; ++tick)
    {
        frame.registry.get<arena::CharacterState>(player).desiredVelocity = unison::Float3{stats.moveSpeed, 0.0F, 0.0F};

        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});
    }

    REQUIRE(frame.registry.get<unison::sim::Transform>(crate).position.x > stood + 0.3F);
}

TEST_CASE("a crate nobody touches stays where it was put")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::CharacterMove move{stats};
    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(move);
    pipeline.add(physics);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    place(frame, assets, arena::kFloor, unison::Float3{0.0F, 0.0F, 0.0F});

    const entt::entity crate = place(frame, assets, arena::kCrate, unison::Float3{1.5F, 1.0F, 0.0F});
    const entt::entity player = standPlayer(frame, stats, unison::Float3{0.0F, 0.5F, 0.0F});

    const float stood = frame.registry.get<unison::sim::Transform>(crate).position.x;

    for (int tick = 0; tick < 120; ++tick)
    {
        frame.registry.get<arena::CharacterState>(player).desiredVelocity = unison::Float3{};

        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});
    }

    REQUIRE(frame.registry.get<unison::sim::Transform>(crate).position.x < stood + 0.05F);
    REQUIRE(frame.registry.get<unison::sim::Transform>(crate).position.x > stood - 0.05F);
}
