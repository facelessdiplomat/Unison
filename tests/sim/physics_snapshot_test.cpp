#include <unison/sim/frame_snapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/allocation_probe.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/character_lifecycle.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/physics_step.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

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

entt::entity spawn(unison::sim::Frame& frame,
                   const unison::sim::AssetRegistry& assets,
                   unison::AssetId definition,
                   const unison::Float3& position)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, position, unison::Quaternion{});
    unison::sim::addBody(frame, assets, entity, definition);

    return entity;
}

void buildArena(unison::sim::Frame& frame, const unison::sim::AssetRegistry& assets)
{
    frame.dt = kTickSeconds;

    spawn(frame, assets, kFloor, unison::Float3{0.0F, 0.0F, 0.0F});
    spawn(frame, assets, kCrate, unison::Float3{0.0F, 3.0F, 0.0F});
    spawn(frame, assets, kCrate, unison::Float3{0.1F, 5.0F, 0.0F});
    spawn(frame, assets, kCrate, unison::Float3{-0.2F, 7.0F, 0.3F});
}

void run(unison::sim::Frame& frame, const unison::sim::SystemPipeline& pipeline, int ticks)
{
    const unison::sim::FrameInputs inputs;

    for (int tick = 0; tick < ticks; ++tick)
    {
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

TEST_CASE("a snapshot carries the physics of the frame it was taken from")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    run(frame, pipeline, 20);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    REQUIRE_FALSE(snapshot.physicsState.empty());
    REQUIRE(snapshot.physicsState == physicsOf(frame));
}

TEST_CASE("a restored frame runs on to the physics it ran to before")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    run(frame, pipeline, 20);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    run(frame, pipeline, 40);

    const std::vector<std::byte> expected = physicsOf(frame);
    const std::uint64_t expectedChecksum = unison::sim::checksumOf(frame);

    unison::sim::restoreSnapshot(snapshot, frame);
    run(frame, pipeline, 40);

    REQUIRE(physicsOf(frame) == expected);
    REQUIRE(unison::sim::checksumOf(frame) == expectedChecksum);
}

TEST_CASE("a restored frame forgets the physics of the ticks it took back")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    run(frame, pipeline, 20);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    const std::vector<std::byte> atSnapshot = physicsOf(frame);

    run(frame, pipeline, 40);

    REQUIRE(physicsOf(frame) != atSnapshot);

    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(physicsOf(frame) == atSnapshot);
}

TEST_CASE("a world that has gone to sleep can be taken back to when it was still moving")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    run(frame, pipeline, 20);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    const std::vector<std::byte> moving = physicsOf(frame);

    run(frame, pipeline, 400);

    const std::vector<std::byte> asleep = physicsOf(frame);

    run(frame, pipeline, 10);

    REQUIRE(physicsOf(frame) == asleep);

    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(physicsOf(frame) == moving);

    run(frame, pipeline, 400);

    REQUIRE(physicsOf(frame) == asleep);
}

TEST_CASE("a body created after a snapshot is gone once the snapshot is restored")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    run(frame, pipeline, 20);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    const std::vector<std::byte> atSnapshot = physicsOf(frame);
    const std::uint32_t countAtSnapshot = frame.physics.bodies().count();

    const entt::entity late = spawn(frame, assets, kCrate, unison::Float3{3.0F, 9.0F, 0.0F});
    const unison::BodyId lateId = frame.registry.get<unison::sim::PhysicsBody>(late).id;

    run(frame, pipeline, 10);
    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE_FALSE(frame.physics.bodies().holds(lateId));
    REQUIRE(frame.physics.bodies().count() == countAtSnapshot);
    REQUIRE(physicsOf(frame) == atSnapshot);
}

TEST_CASE("a body destroyed after a snapshot comes back with the state it had")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    const entt::entity doomed = spawn(frame, assets, kCrate, unison::Float3{2.0F, 4.0F, 0.0F});

    run(frame, pipeline, 20);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    const std::vector<std::byte> atSnapshot = physicsOf(frame);
    const unison::BodyId doomedId = frame.registry.get<unison::sim::PhysicsBody>(doomed).id;

    unison::sim::removeBody(frame, doomed);

    REQUIRE_FALSE(frame.physics.bodies().holds(doomedId));

    run(frame, pipeline, 10);
    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(frame.physics.bodies().holds(doomedId));
    REQUIRE(physicsOf(frame) == atSnapshot);
}

TEST_CASE("the surface a body was given back is the one the snapshot knew")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    const entt::entity crate = spawn(frame, assets, kCrate, unison::Float3{2.0F, 4.0F, 0.0F});
    const unison::BodyId id = frame.registry.get<unison::sim::PhysicsBody>(crate).id;

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    frame.registry.get<unison::sim::BodyDefinition>(crate).friction = 0.9F;
    frame.registry.get<unison::sim::BodyDefinition>(crate).restitution = 0.7F;
    unison::sim::applyBodyProperties(frame, crate);

    REQUIRE(frame.physics.bodies().frictionOf(id) == 0.9F);
    REQUIRE(frame.physics.bodies().restitutionOf(id) == 0.7F);

    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(frame.physics.bodies().frictionOf(id) == 0.2F);
    REQUIRE(frame.physics.bodies().restitutionOf(id) == 0.0F);
}

TEST_CASE("the way a body was allowed to move is given back with it")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    const entt::entity crate = spawn(frame, assets, kCrate, unison::Float3{2.0F, 4.0F, 0.0F});
    const unison::BodyId id = frame.registry.get<unison::sim::PhysicsBody>(crate).id;

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    frame.registry.get<unison::sim::BodyDefinition>(crate).motion = unison::sim::BodyMotion::Kinematic;
    unison::sim::applyBodyProperties(frame, crate);

    REQUIRE(frame.physics.bodies().motionOf(id) == unison::sim::BodyMotion::Kinematic);

    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(frame.physics.bodies().motionOf(id) == unison::sim::BodyMotion::Dynamic);
}

TEST_CASE("a body rebuilt on restore is rebuilt with the surface it had")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::Frame frame;
    buildArena(frame, assets);

    const entt::entity crate = spawn(frame, assets, kCrate, unison::Float3{2.0F, 4.0F, 0.0F});
    const unison::BodyId id = frame.registry.get<unison::sim::PhysicsBody>(crate).id;

    frame.registry.get<unison::sim::BodyDefinition>(crate).friction = 0.4F;
    unison::sim::applyBodyProperties(frame, crate);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    unison::sim::removeBody(frame, crate);
    unison::sim::restoreSnapshot(snapshot, frame);

    REQUIRE(frame.physics.bodies().holds(id));
    REQUIRE(frame.physics.bodies().frictionOf(id) == 0.4F);
}

TEST_CASE("reconciling a frame again while it still holds its bodies and characters allocates nothing")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);
    unison::sim::Frame frame;
    buildArena(frame, assets);
    const entt::entity walker = frame.registry.create();
    frame.registry.emplace<unison::sim::Transform>(walker, unison::Float3{3.0F, 1.0F, 0.0F}, unison::Quaternion{});
    unison::sim::addCharacter(frame, walker, unison::sim::CharacterController{});
    unison::sim::reconcileBodies(frame);
    unison::sim::reconcileCharacters(frame);

    const unison::test::AllocationProbe probe;
    unison::sim::reconcileBodies(frame);
    unison::sim::reconcileCharacters(frame);

    REQUIRE(probe.allocations() == 0U);
}
