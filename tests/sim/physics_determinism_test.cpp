#include <unison/core/hasher.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/advance_frame.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_snapshot.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/physics_step.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace
{

constexpr unison::AssetId kCrate = unison::makeAssetId("crate");
constexpr unison::AssetId kFloor = unison::makeAssetId("floor");
constexpr float kTickSeconds = 1.0F / 60.0F;
constexpr int kCrateCount = 50;
constexpr int kFrameCount = 600;
constexpr std::uint64_t kGoldenPhysicsChecksum = 0x214BC6AEDC1EFBB3U;

void defineArena(unison::sim::AssetRegistry& assets)
{
    unison::sim::BodyDefinition crate;
    crate.halfExtents = unison::Float3{0.5F, 0.5F, 0.5F};
    crate.motion = unison::sim::BodyMotion::Dynamic;
    crate.layer = unison::sim::PhysicsLayer::Moving;
    crate.friction = 0.4F;
    crate.restitution = 0.2F;

    unison::sim::BodyDefinition floor;
    floor.halfExtents = unison::Float3{50.0F, 0.5F, 50.0F};
    floor.friction = 0.6F;

    assets.add<unison::sim::BodyDefinition>(kCrate, crate);
    assets.add<unison::sim::BodyDefinition>(kFloor, floor);
    assets.freeze();
}

void spawn(unison::sim::Frame& frame,
           const unison::sim::AssetRegistry& assets,
           unison::AssetId definition,
           const unison::Float3& position)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, position, unison::Quaternion{});
    unison::sim::addBody(frame, assets, entity, definition);
}

void buildPile(unison::sim::Frame& frame, const unison::sim::AssetRegistry& assets)
{
    frame.dt = kTickSeconds;

    spawn(frame, assets, kFloor, unison::Float3{0.0F, 0.0F, 0.0F});

    for (int index = 0; index < kCrateCount; ++index)
    {
        const float column = static_cast<float>(index % 5);
        const float row = static_cast<float>((index / 5) % 5);
        const float layer = static_cast<float>(index / 25);
        const float lean = static_cast<float>(index) * 0.01F;

        spawn(
            frame, assets, kCrate, unison::Float3{column * 1.1F - 2.2F + lean, 1.5F + layer * 1.4F, row * 1.1F - 2.2F});
    }
}

void run(unison::sim::Frame& frame, const unison::sim::SystemPipeline& pipeline, int frames)
{
    const unison::sim::FrameInputs inputs;

    for (int frameNumber = 0; frameNumber < frames; ++frameNumber)
    {
        unison::sim::advanceFrame(frame, pipeline, inputs);
    }
}

std::uint64_t physicsChecksumOf(const unison::sim::Frame& frame)
{
    std::vector<std::byte> state;
    frame.physics.saveState(state);

    unison::Hasher hasher;
    hasher.add(std::span<const std::byte>{state});

    return hasher.finish();
}

}

TEST_CASE("fifty falling boxes agree on every frame of two runs")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    unison::sim::Frame first;
    buildPile(first, assets);

    unison::sim::Frame second;
    buildPile(second, assets);

    const unison::sim::FrameInputs inputs;

    for (int frameNumber = 0; frameNumber < kFrameCount; ++frameNumber)
    {
        unison::sim::advanceFrame(first, pipeline, inputs);
        unison::sim::advanceFrame(second, pipeline, inputs);

        if (unison::sim::checksumOf(first) != unison::sim::checksumOf(second))
        {
            FAIL("the runs parted ways on frame " << frameNumber);
        }
    }

    REQUIRE(unison::sim::checksumOf(first) == unison::sim::checksumOf(second));
}

TEST_CASE("fifty falling boxes come to rest where they always have")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    unison::sim::Frame frame;
    buildPile(frame, assets);

    run(frame, pipeline, kFrameCount);

    REQUIRE(physicsChecksumOf(frame) == kGoldenPhysicsChecksum);
}

TEST_CASE("fifty falling boxes rolled back halfway come to rest where they would have")
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    unison::sim::PhysicsStep physics;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(physics);

    unison::sim::Frame frame;
    buildPile(frame, assets);

    run(frame, pipeline, kFrameCount / 2);

    unison::sim::FrameSnapshot halfway;
    unison::sim::takeSnapshot(frame, halfway);

    run(frame, pipeline, kFrameCount / 2);

    const std::uint64_t resting = unison::sim::checksumOf(frame);

    unison::sim::restoreSnapshot(halfway, frame);
    run(frame, pipeline, kFrameCount / 2);

    REQUIRE(unison::sim::checksumOf(frame) == resting);
}
