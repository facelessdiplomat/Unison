#include <arena/hits.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/assets.hpp>
#include <arena/components.hpp>
#include <arena/events.hpp>
#include <arena/respawn.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/character_lifecycle.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

constexpr float kTickSeconds = 1.0F / 60.0F;

entt::entity standPlayer(unison::sim::Frame& frame, const arena::PlayerStats& stats, const unison::Float3& at)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, at, unison::Quaternion{});
    frame.registry.emplace<arena::Health>(entity, stats.maxHealth, entt::null);
    frame.registry.emplace<arena::PlayerSlot>(entity, std::uint8_t{0});
    frame.registry.emplace<arena::CharacterState>(entity);

    unison::sim::CharacterController capsule;
    capsule.radius = stats.capsuleRadius;
    capsule.halfHeight = stats.capsuleHalfHeight;

    unison::sim::addCharacter(frame, entity, capsule);

    return entity;
}

entt::entity fire(unison::sim::Frame& frame,
                  const arena::ProjectileStats& stats,
                  entt::entity firedBy,
                  const unison::Float3& from,
                  const unison::Float3& towards)
{
    const entt::entity shot = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(shot, from, unison::Quaternion{});
    frame.registry.emplace<arena::Projectile>(
        shot,
        firedBy,
        stats.damage,
        unison::Float3{towards.x * stats.speed, towards.y * stats.speed, towards.z * stats.speed});
    frame.registry.emplace<arena::Lifetime>(shot, stats.lifetimeFrames);

    return shot;
}

void wall(unison::sim::Frame& frame, const unison::sim::AssetRegistry& assets, const unison::Float3& at)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, at, unison::Quaternion{});
    unison::sim::addBody(frame, assets, entity, arena::kWall);
}

std::size_t hitEventsIn(const unison::sim::Frame& frame)
{
    std::size_t hits = 0;

    for (std::size_t index = 0; index < frame.events.size(); ++index)
    {
        if (frame.events.keyAt(index).typeId == unison::sim::EventTraits<arena::Hit>::typeId)
        {
            ++hits;
        }
    }

    return hits;
}

}

TEST_CASE("a shot that reaches a player takes health off them once")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& player = assets.get<arena::PlayerStats>(arena::kPlayerStats);
    const arena::ProjectileStats& shot = assets.get<arena::ProjectileStats>(arena::kProjectileStats);

    arena::Hits hits{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(hits);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    const entt::entity target = standPlayer(frame, player, unison::Float3{4.0F, 0.0F, 0.0F});
    const entt::entity flying =
        fire(frame, shot, entt::null, unison::Float3{0.0F, 1.0F, 0.0F}, unison::Float3{1.0F, 0.0F, 0.0F});

    std::size_t hitEvents = 0;

    for (int tick = 0; tick < 30; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});
        hitEvents += hitEventsIn(frame);
    }

    REQUIRE(hitEvents == 1U);
    REQUIRE(frame.registry.get<arena::Health>(target).points == player.maxHealth - shot.damage);
    REQUIRE_FALSE(frame.registry.valid(flying));
}

TEST_CASE("a shot that reaches a wall goes no further")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& player = assets.get<arena::PlayerStats>(arena::kPlayerStats);
    const arena::ProjectileStats& shot = assets.get<arena::ProjectileStats>(arena::kProjectileStats);

    arena::Hits hits{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(hits);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    wall(frame, assets, unison::Float3{2.0F, 1.0F, 0.0F});

    const entt::entity target = standPlayer(frame, player, unison::Float3{4.0F, 0.0F, 0.0F});
    const entt::entity flying =
        fire(frame, shot, entt::null, unison::Float3{0.0F, 1.0F, 0.0F}, unison::Float3{1.0F, 0.0F, 0.0F});

    for (int tick = 0; tick < 30; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});
    }

    REQUIRE_FALSE(frame.registry.valid(flying));
    REQUIRE(frame.registry.get<arena::Health>(target).points == player.maxHealth);
}

TEST_CASE("a shot that reaches nothing flies on")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::ProjectileStats& shot = assets.get<arena::ProjectileStats>(arena::kProjectileStats);

    arena::Hits hits{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(hits);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    const entt::entity flying =
        fire(frame, shot, entt::null, unison::Float3{0.0F, 1.0F, 0.0F}, unison::Float3{1.0F, 0.0F, 0.0F});

    unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});

    REQUIRE(frame.registry.valid(flying));
    REQUIRE(frame.registry.get<unison::sim::Transform>(flying).position.x > shot.speed * kTickSeconds * 0.9F);
}

TEST_CASE("a shot cannot reach the player who fired it")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& player = assets.get<arena::PlayerStats>(arena::kPlayerStats);
    const arena::ProjectileStats& shot = assets.get<arena::ProjectileStats>(arena::kProjectileStats);

    arena::Hits hits{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(hits);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    const entt::entity shooter = standPlayer(frame, player, unison::Float3{0.0F, 0.0F, 0.0F});
    const entt::entity flying =
        fire(frame, shot, shooter, unison::Float3{0.0F, 1.0F, 0.0F}, unison::Float3{1.0F, 0.0F, 0.0F});

    unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});

    REQUIRE(frame.registry.valid(flying));
    REQUIRE(frame.registry.get<arena::Health>(shooter).points == player.maxHealth);
}

TEST_CASE("a shot that reaches a player tells the view where it reached them")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& player = assets.get<arena::PlayerStats>(arena::kPlayerStats);
    const arena::ProjectileStats& shot = assets.get<arena::ProjectileStats>(arena::kProjectileStats);

    arena::Hits hits{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(hits);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    const entt::entity target = standPlayer(frame, player, unison::Float3{2.0F, 0.0F, 0.0F});
    const entt::entity flying =
        fire(frame, shot, entt::null, unison::Float3{0.0F, 1.0F, 0.0F}, unison::Float3{1.0F, 0.0F, 0.0F});

    arena::Hit reported;

    for (int tick = 0; tick < 30 && frame.registry.valid(flying); ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});

        for (std::size_t index = 0; index < frame.events.size(); ++index)
        {
            if (frame.events.keyAt(index).typeId == unison::sim::EventTraits<arena::Hit>::typeId)
            {
                reported = frame.events.payloadAt<arena::Hit>(index);
            }
        }
    }

    REQUIRE(reported.target == target);
    REQUIRE(reported.shot == flying);
    REQUIRE(reported.at.x > 1.0F);
    REQUIRE(reported.at.x < 3.0F);
}

TEST_CASE("a player shot to death names their killer")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& player = assets.get<arena::PlayerStats>(arena::kPlayerStats);
    const arena::ProjectileStats& shot = assets.get<arena::ProjectileStats>(arena::kProjectileStats);

    arena::Hits hits{assets};
    arena::Respawn respawn{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(hits);
    pipeline.add(respawn);

    unison::sim::Frame frame;
    frame.dt = kTickSeconds;

    const entt::entity shooter = standPlayer(frame, player, unison::Float3{0.0F, 0.0F, 0.0F});
    const entt::entity target = standPlayer(frame, player, unison::Float3{3.0F, 0.0F, 0.0F});

    frame.registry.get<arena::Health>(target) = arena::Health{shot.damage, entt::null};

    fire(frame, shot, shooter, unison::Float3{0.5F, 1.0F, 0.0F}, unison::Float3{1.0F, 0.0F, 0.0F});

    arena::Died died;

    for (int tick = 0; tick < 30; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});

        for (std::size_t index = 0; index < frame.events.size(); ++index)
        {
            if (frame.events.keyAt(index).typeId == unison::sim::EventTraits<arena::Died>::typeId)
            {
                died = frame.events.payloadAt<arena::Died>(index);
            }
        }
    }

    REQUIRE(died.player == target);
    REQUIRE(died.killedBy == shooter);
}
