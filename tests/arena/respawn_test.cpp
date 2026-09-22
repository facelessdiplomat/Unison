#include <arena/respawn.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/assets.hpp>
#include <arena/components.hpp>
#include <arena/events.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/character_lifecycle.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

entt::entity joinMatch(unison::sim::Frame& frame, const arena::PlayerStats& stats, std::uint8_t slot)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, unison::Float3{0.0F, 0.0F, 0.0F}, unison::Quaternion{});
    frame.registry.emplace<arena::PlayerSlot>(entity, slot);
    frame.registry.emplace<arena::CharacterState>(entity);
    frame.registry.emplace<arena::Health>(entity, stats.maxHealth, entt::null);
    frame.registry.emplace<arena::Weapon>(entity);

    unison::sim::CharacterController capsule;
    capsule.radius = stats.capsuleRadius;
    capsule.halfHeight = stats.capsuleHalfHeight;

    unison::sim::addCharacter(frame, entity, capsule);

    return entity;
}

bool raised(const unison::sim::Frame& frame, std::uint32_t typeId)
{
    for (std::size_t index = 0; index < frame.events.size(); ++index)
    {
        if (frame.events.keyAt(index).typeId == typeId)
        {
            return true;
        }
    }

    return false;
}

void run(unison::sim::Frame& frame, const unison::sim::SystemPipeline& pipeline, std::uint32_t ticks)
{
    for (std::uint32_t tick = 0; tick < ticks; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});
    }
}

}

TEST_CASE("a player whose health runs out leaves the world and starts waiting")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::Respawn respawn{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(respawn);

    unison::sim::Frame frame;
    const entt::entity player = joinMatch(frame, stats, 0U);
    const entt::entity killer = joinMatch(frame, stats, 1U);

    frame.registry.get<arena::Health>(player) = arena::Health{0, killer};

    unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});

    REQUIRE(raised(frame, unison::sim::EventTraits<arena::Died>::typeId));
    REQUIRE_FALSE(frame.registry.all_of<unison::sim::CharacterController>(player));
    REQUIRE(frame.registry.all_of<arena::RespawnTimer>(player));
    REQUIRE(frame.registry.get<arena::RespawnTimer>(player).framesLeft == stats.respawnFrames);
}

TEST_CASE("a death names the player who caused it")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::Respawn respawn{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(respawn);

    unison::sim::Frame frame;
    const entt::entity player = joinMatch(frame, stats, 0U);
    const entt::entity killer = joinMatch(frame, stats, 1U);

    frame.registry.get<arena::Health>(player) = arena::Health{-10, killer};

    unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});

    const arena::Died died = frame.events.payloadAt<arena::Died>(0);

    REQUIRE(died.player == player);
    REQUIRE(died.killedBy == killer);
}

TEST_CASE("a player comes back whole once the timer has run out")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);
    const arena::SpawnPoints& points = assets.get<arena::SpawnPoints>(arena::kSpawnPoints);

    arena::Respawn respawn{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(respawn);

    unison::sim::Frame frame;
    const entt::entity player = joinMatch(frame, stats, 0U);

    frame.registry.get<arena::Health>(player) = arena::Health{0, entt::null};

    run(frame, pipeline, stats.respawnFrames + 1U);

    REQUIRE(raised(frame, unison::sim::EventTraits<arena::Respawned>::typeId));
    REQUIRE(frame.registry.all_of<unison::sim::CharacterController>(player));
    REQUIRE_FALSE(frame.registry.all_of<arena::RespawnTimer>(player));
    REQUIRE(frame.registry.get<arena::Health>(player).points == stats.maxHealth);

    const unison::Float3 standing = frame.registry.get<unison::sim::Transform>(player).position;

    bool atASpawnPoint = false;

    for (const unison::Float3& point : points.positions)
    {
        atASpawnPoint = atASpawnPoint || (standing.x == point.x && standing.z == point.z);
    }

    REQUIRE(atASpawnPoint);
}

TEST_CASE("a player is not back before the timer has run out")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::Respawn respawn{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(respawn);

    unison::sim::Frame frame;
    const entt::entity player = joinMatch(frame, stats, 0U);

    frame.registry.get<arena::Health>(player) = arena::Health{0, entt::null};

    run(frame, pipeline, stats.respawnFrames);

    REQUIRE(frame.registry.all_of<arena::RespawnTimer>(player));
    REQUIRE_FALSE(frame.registry.all_of<unison::sim::CharacterController>(player));
}

TEST_CASE("a player still on their feet is left alone")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::Respawn respawn{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(respawn);

    unison::sim::Frame frame;
    const entt::entity player = joinMatch(frame, stats, 0U);

    run(frame, pipeline, 30U);

    REQUIRE(frame.registry.all_of<unison::sim::CharacterController>(player));
    REQUIRE_FALSE(frame.registry.all_of<arena::RespawnTimer>(player));
    REQUIRE(frame.events.size() == 0U);
}

TEST_CASE("two matches draw the same spawn point for the same death")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::PlayerStats& stats = assets.get<arena::PlayerStats>(arena::kPlayerStats);

    arena::Respawn respawn{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(respawn);

    unison::sim::Frame one;
    unison::sim::Frame other;

    const entt::entity first = joinMatch(one, stats, 0U);
    const entt::entity second = joinMatch(other, stats, 0U);

    one.registry.get<arena::Health>(first) = arena::Health{0, entt::null};
    other.registry.get<arena::Health>(second) = arena::Health{0, entt::null};

    run(one, pipeline, stats.respawnFrames + 1U);
    run(other, pipeline, stats.respawnFrames + 1U);

    REQUIRE(one.registry.get<unison::sim::Transform>(first).position.x ==
            other.registry.get<unison::sim::Transform>(second).position.x);
    REQUIRE(one.registry.get<unison::sim::Transform>(first).position.z ==
            other.registry.get<unison::sim::Transform>(second).position.z);
}
