#include <arena/respawn.hpp>

#include <arena/assets.hpp>
#include <arena/components.hpp>
#include <arena/events.hpp>

#include <unison/sim/character_lifecycle.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <vector>

namespace arena
{

namespace
{

void takeOutOfTheWorld(unison::sim::Frame& frame, entt::entity player, const PlayerStats& stats)
{
    const entt::entity killedBy = frame.registry.get<Health>(player).lastHitBy;

    frame.events.raise(frame.frameNumber, Died{player, killedBy});

    unison::sim::removeCharacter(frame, player);

    frame.registry.emplace<RespawnTimer>(player, stats.respawnFrames);
    frame.registry.emplace<Killed>(player, killedBy);
}

void putBackIntoTheWorld(unison::sim::Frame& frame,
                         entt::entity player,
                         const PlayerStats& stats,
                         const SpawnPoints& points)
{
    const auto drawn =
        static_cast<std::size_t>(frame.globals.rng.nextInRange(0, static_cast<std::int32_t>(kPlayerCount) - 1));

    frame.registry.get<unison::sim::Transform>(player) =
        unison::sim::Transform{points.positions[drawn], unison::Quaternion{}};
    frame.registry.get<Health>(player) = Health{stats.maxHealth, entt::null};
    frame.registry.get<CharacterState>(player) = CharacterState{unison::Float3{}, points.yaws[drawn], 0.0F};

    unison::sim::CharacterController capsule;
    capsule.radius = stats.capsuleRadius;
    capsule.halfHeight = stats.capsuleHalfHeight;

    unison::sim::addCharacter(frame, player, capsule);

    frame.registry.erase<RespawnTimer>(player);
    frame.events.raise(frame.frameNumber, Respawned{player});
}

}

Respawn::Respawn(const unison::sim::AssetRegistry& assets) : assets{assets}
{
}

void Respawn::update(unison::sim::Frame& frame, const unison::sim::FrameInputs&)
{
    const PlayerStats& stats = assets.get<PlayerStats>(kPlayerStats);

    std::vector<entt::entity> died;
    std::vector<entt::entity> returning;

    for (const auto [entity, slot, health] : frame.registry.view<const PlayerSlot, const Health>().each())
    {
        if (health.points <= 0 && !frame.registry.all_of<RespawnTimer>(entity))
        {
            died.push_back(entity);
        }
    }

    for (const auto [entity, timer] : frame.registry.view<RespawnTimer>().each())
    {
        if (timer.framesLeft > 0U)
        {
            --timer.framesLeft;
        }

        if (timer.framesLeft == 0U)
        {
            returning.push_back(entity);
        }
    }

    for (const entt::entity player : died)
    {
        takeOutOfTheWorld(frame, player, stats);
    }

    for (const entt::entity player : returning)
    {
        putBackIntoTheWorld(frame, player, stats, assets.get<SpawnPoints>(kSpawnPoints));
    }
}

std::string_view Respawn::name() const
{
    return "Respawn";
}

}
