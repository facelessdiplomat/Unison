#include <arena/apply_input.hpp>

#include <unison/core/math.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/assets.hpp>
#include <arena/components.hpp>
#include <unison/sim/advance_frame.hpp>

#include <entt/entity/registry.hpp>

#include <cstdint>

namespace
{

constexpr std::int16_t kQuarterTurn = 16384;

entt::entity joinAs(unison::sim::Frame& frame, std::uint8_t slot)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<arena::PlayerSlot>(entity, slot);
    frame.registry.emplace<arena::CharacterState>(entity);

    return entity;
}

unison::sim::FrameInputs asked(std::uint8_t slot, const arena::ArenaInput& input)
{
    unison::sim::FrameInputs inputs;
    inputs.set(slot, input, unison::sim::InputFlags::Present);

    return inputs;
}

}

TEST_CASE("a player asking to run forward runs forward at full speed")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::ApplyInput applyInput{stats};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(applyInput);

    unison::sim::Frame frame;
    const entt::entity player = joinAs(frame, 0U);

    arena::ArenaInput input;
    input.moveY = 127;

    unison::sim::advanceFrame(frame, pipeline, asked(0U, input));

    const arena::CharacterState& state = frame.registry.get<arena::CharacterState>(player);

    REQUIRE(state.desiredVelocity.z > stats.moveSpeed - 0.001F);
    REQUIRE(state.desiredVelocity.x < 0.001F);
    REQUIRE(state.desiredVelocity.x > -0.001F);
    REQUIRE(state.desiredVelocity.y == 0.0F);
}

TEST_CASE("a player running forward runs the way they are facing")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::ApplyInput applyInput{stats};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(applyInput);

    unison::sim::Frame frame;
    const entt::entity player = joinAs(frame, 0U);

    arena::ArenaInput input;
    input.moveY = 127;
    input.yaw = kQuarterTurn;

    unison::sim::advanceFrame(frame, pipeline, asked(0U, input));

    const arena::CharacterState& state = frame.registry.get<arena::CharacterState>(player);

    REQUIRE(state.desiredVelocity.x > stats.moveSpeed - 0.001F);
    REQUIRE(state.desiredVelocity.z < 0.001F);
    REQUIRE(state.yaw > 1.57F);
    REQUIRE(state.yaw < 1.58F);
}

TEST_CASE("a player running cornerwise is no faster than one running straight")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::ApplyInput applyInput{stats};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(applyInput);

    unison::sim::Frame frame;
    const entt::entity player = joinAs(frame, 0U);

    arena::ArenaInput input;
    input.moveX = 127;
    input.moveY = 127;

    unison::sim::advanceFrame(frame, pipeline, asked(0U, input));

    const unison::Float3 velocity = frame.registry.get<arena::CharacterState>(player).desiredVelocity;
    const float speed = unison::math::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);

    REQUIRE(speed < stats.moveSpeed + 0.001F);
    REQUIRE(speed > stats.moveSpeed - 0.001F);
}

TEST_CASE("a player asking for nothing stands still")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    arena::ApplyInput applyInput{assets.get<arena::PlayerStats>(arena::kPlayerStats)};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(applyInput);

    unison::sim::Frame frame;
    const entt::entity player = joinAs(frame, 0U);

    unison::sim::advanceFrame(frame, pipeline, asked(0U, arena::ArenaInput{}));

    const unison::Float3 velocity = frame.registry.get<arena::CharacterState>(player).desiredVelocity;

    REQUIRE(velocity.x == 0.0F);
    REQUIRE(velocity.z == 0.0F);
}

TEST_CASE("every player is moved by the input of their own slot")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    arena::ApplyInput applyInput{assets.get<arena::PlayerStats>(arena::kPlayerStats)};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(applyInput);

    unison::sim::Frame frame;
    const entt::entity first = joinAs(frame, 0U);
    const entt::entity second = joinAs(frame, 1U);

    arena::ArenaInput input;
    input.moveY = 127;

    unison::sim::advanceFrame(frame, pipeline, asked(1U, input));

    REQUIRE(frame.registry.get<arena::CharacterState>(first).desiredVelocity.z == 0.0F);
    REQUIRE(frame.registry.get<arena::CharacterState>(second).desiredVelocity.z > 1.0F);
}
