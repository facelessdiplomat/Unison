#include <unison/sim/character_lifecycle.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/advance_frame.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/character.hpp>
#include <unison/sim/frame_snapshot.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/physics_step.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <vector>

namespace
{

constexpr unison::AssetId kFloor = unison::makeAssetId("floor");
constexpr unison::AssetId kWall = unison::makeAssetId("wall");
constexpr float kTickSeconds = 1.0F / 60.0F;

void defineArena(unison::sim::AssetRegistry& assets)
{
    unison::sim::BodyDefinition floor;
    floor.halfExtents = unison::Float3{50.0F, 0.5F, 50.0F};

    unison::sim::BodyDefinition wall;
    wall.halfExtents = unison::Float3{0.5F, 2.0F, 5.0F};

    assets.add<unison::sim::BodyDefinition>(kFloor, floor);
    assets.add<unison::sim::BodyDefinition>(kWall, wall);
    assets.freeze();
}

void spawnBody(unison::sim::Frame& frame,
               const unison::sim::AssetRegistry& assets,
               unison::AssetId definition,
               const unison::Float3& position)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, position, unison::Quaternion{});
    unison::sim::addBody(frame, assets, entity, definition);
}

entt::entity spawnCharacter(unison::sim::Frame& frame, const unison::Float3& position)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, position, unison::Quaternion{});
    unison::sim::addCharacter(frame, entity);

    return entity;
}

void walk(unison::sim::Frame& frame,
          const unison::sim::SystemPipeline& pipeline,
          entt::entity walker,
          const unison::Float3& velocity,
          int ticks)
{
    const unison::sim::FrameInputs inputs;

    for (int tick = 0; tick < ticks; ++tick)
    {
        frame.registry.get<unison::sim::CharacterController>(walker).velocity = velocity;
        unison::sim::advanceFrame(frame, pipeline, inputs);
    }
}

std::vector<std::byte> physicsOf(const unison::sim::Frame& frame)
{
    std::vector<std::byte> bytes;
    frame.physics.saveState(bytes);

    return bytes;
}

}

TEST_CASE("a character stands on the floor it was put on")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    spawnBody(frame, assets, kFloor, unison::Float3{0.0F, 0.0F, 0.0F});

    const entt::entity walker = spawnCharacter(frame, unison::Float3{0.0F, 0.5F, 0.0F});

    walk(frame, pipeline, walker, unison::Float3{0.0F, -1.0F, 0.0F}, 5);

    REQUIRE(frame.registry.get<unison::sim::CharacterController>(walker).ground == unison::sim::GroundState::OnGround);
}

TEST_CASE("a character walking forward gets there")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    spawnBody(frame, assets, kFloor, unison::Float3{0.0F, 0.0F, 0.0F});

    const entt::entity walker = spawnCharacter(frame, unison::Float3{0.0F, 0.5F, 0.0F});

    walk(frame, pipeline, walker, unison::Float3{2.0F, -1.0F, 0.0F}, 30);

    REQUIRE(frame.registry.get<unison::sim::Transform>(walker).position.x > 0.5F);
}

TEST_CASE("a character walking into a wall stops at it")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    spawnBody(frame, assets, kFloor, unison::Float3{0.0F, 0.0F, 0.0F});
    spawnBody(frame, assets, kWall, unison::Float3{3.0F, 2.0F, 0.0F});

    const entt::entity walker = spawnCharacter(frame, unison::Float3{0.0F, 0.5F, 0.0F});

    walk(frame, pipeline, walker, unison::Float3{4.0F, -1.0F, 0.0F}, 120);

    const float stopped = frame.registry.get<unison::sim::Transform>(walker).position.x;

    REQUIRE(stopped > 1.0F);
    REQUIRE(stopped < 2.5F);
}

TEST_CASE("a character comes back from a snapshot exactly as it stood")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    spawnBody(frame, assets, kFloor, unison::Float3{0.0F, 0.0F, 0.0F});

    const entt::entity walker = spawnCharacter(frame, unison::Float3{0.0F, 0.5F, 0.0F});

    walk(frame, pipeline, walker, unison::Float3{2.0F, -1.0F, 0.0F}, 20);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    const std::vector<std::byte> atSnapshot = physicsOf(frame);
    const float standing = frame.registry.get<unison::sim::Transform>(walker).position.x;

    walk(frame, pipeline, walker, unison::Float3{2.0F, -1.0F, 0.0F}, 20);

    const std::vector<std::byte> walkedOn = physicsOf(frame);
    const float arrived = frame.registry.get<unison::sim::Transform>(walker).position.x;

    REQUIRE(arrived > standing);

    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(physicsOf(frame) == atSnapshot);
    REQUIRE(frame.registry.get<unison::sim::Transform>(walker).position.x == standing);

    walk(frame, pipeline, walker, unison::Float3{2.0F, -1.0F, 0.0F}, 20);

    REQUIRE(physicsOf(frame) == walkedOn);
    REQUIRE(frame.registry.get<unison::sim::Transform>(walker).position.x == arrived);
}

TEST_CASE("a character taken away leaves the world without it")
{
    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    const entt::entity walker = spawnCharacter(frame, unison::Float3{0.0F, 0.5F, 0.0F});
    const unison::BodyId id = frame.registry.get<unison::sim::CharacterController>(walker).id;

    REQUIRE(frame.physics.characters().holds(id));

    unison::sim::removeCharacter(frame, walker);

    REQUIRE_FALSE(frame.physics.characters().holds(id));
    REQUIRE_FALSE(frame.registry.all_of<unison::sim::CharacterController>(walker));
}
