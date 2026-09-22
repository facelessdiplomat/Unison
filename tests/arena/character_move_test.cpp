#include <arena/character_move.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/arena_input.hpp>
#include <arena/assets.hpp>
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

entt::entity standPlayer(unison::sim::Frame& frame, const arena::PlayerStats& stats)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, unison::Float3{0.0F, 0.5F, 0.0F}, unison::Quaternion{});
    frame.registry.emplace<arena::PlayerSlot>(entity, std::uint8_t{0});
    frame.registry.emplace<arena::CharacterState>(entity);

    unison::sim::CharacterController capsule;
    capsule.radius = stats.capsuleRadius;
    capsule.halfHeight = stats.capsuleHalfHeight;

    unison::sim::addCharacter(frame, entity, capsule);

    return entity;
}

void layFloor(unison::sim::Frame& frame, const unison::sim::AssetRegistry& assets)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, unison::Float3{0.0F, 0.0F, 0.0F}, unison::Quaternion{});
    unison::sim::addBody(frame, assets, entity, arena::kFloor);
}

unison::sim::FrameInputs held(bool jump)
{
    arena::ArenaInput input;

    if (jump)
    {
        input.buttons = static_cast<std::uint16_t>(arena::Button::Jump);
    }

    unison::sim::FrameInputs inputs;
    inputs.set(0U, input, unison::sim::InputFlags::Present);

    return inputs;
}

float heightOf(const unison::sim::Frame& frame, entt::entity player)
{
    return frame.registry.get<unison::sim::Transform>(player).position.y;
}

unison::sim::GroundState groundOf(const unison::sim::Frame& frame, entt::entity player)
{
    return frame.registry.get<unison::sim::CharacterController>(player).ground;
}

}

TEST_CASE("a player standing on the floor stays where they stand")
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

    layFloor(frame, assets);

    const entt::entity player = standPlayer(frame, stats);

    for (int tick = 0; tick < 60; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, held(false));
    }

    REQUIRE(heightOf(frame, player) > 0.4F);
    REQUIRE(heightOf(frame, player) < 0.6F);
    REQUIRE(groundOf(frame, player) == unison::sim::GroundState::OnGround);
}

TEST_CASE("a player who jumps leaves the ground and comes back down")
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

    layFloor(frame, assets);

    const entt::entity player = standPlayer(frame, stats);

    for (int tick = 0; tick < 10; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, held(false));
    }

    const float standing = heightOf(frame, player);

    unison::sim::advanceFrame(frame, pipeline, held(true));

    for (int tick = 0; tick < 15; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, held(false));
    }

    REQUIRE(heightOf(frame, player) > standing + 0.5F);
    REQUIRE(groundOf(frame, player) != unison::sim::GroundState::OnGround);

    for (int tick = 0; tick < 45; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, held(false));
    }

    REQUIRE(groundOf(frame, player) == unison::sim::GroundState::OnGround);
    REQUIRE(heightOf(frame, player) < standing + 0.1F);
}

TEST_CASE("a player holding jump does not climb the sky")
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

    layFloor(frame, assets);

    const entt::entity player = standPlayer(frame, stats);

    float highest = 0.0F;

    for (int tick = 0; tick < 300; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, held(true));

        highest = heightOf(frame, player) > highest ? heightOf(frame, player) : highest;
    }

    REQUIRE(highest < 2.5F);
}

TEST_CASE("a player walks where their character state points")
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

    layFloor(frame, assets);

    const entt::entity player = standPlayer(frame, stats);

    for (int tick = 0; tick < 30; ++tick)
    {
        frame.registry.get<arena::CharacterState>(player).desiredVelocity = unison::Float3{stats.moveSpeed, 0.0F, 0.0F};

        unison::sim::advanceFrame(frame, pipeline, held(false));
    }

    REQUIRE(frame.registry.get<unison::sim::Transform>(player).position.x > 1.0F);
}
