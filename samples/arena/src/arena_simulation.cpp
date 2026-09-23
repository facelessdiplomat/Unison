#include <arena/arena_simulation.hpp>

#include <arena/components.hpp>

#include <unison/core/contract.hpp>
#include <unison/core/math.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/character_lifecycle.hpp>
#include <unison/sim/entity_lifecycle.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <array>
#include <cstdint>

namespace arena
{

namespace
{

constexpr float kWallDistance = 12.0F;
constexpr float kWallHeight = 2.0F;

unison::sim::AssetRegistry definedArena()
{
    unison::sim::AssetRegistry assets;
    defineArena(assets);

    return assets;
}

void place(unison::sim::Frame& frame,
           const unison::sim::AssetRegistry& assets,
           unison::AssetId definition,
           const unison::Float3& at,
           const unison::Quaternion& facing)
{
    const entt::entity entity = unison::sim::createEntity(frame);

    frame.registry.emplace<unison::sim::Transform>(entity, at, facing);
    unison::sim::addBody(frame, assets, entity, definition);
}

unison::Quaternion turnedAboutY(float angle)
{
    return unison::Quaternion{0.0F, unison::math::sin(angle * 0.5F), 0.0F, unison::math::cos(angle * 0.5F)};
}

void layOutTheArena(unison::sim::Frame& frame, const unison::sim::AssetRegistry& assets)
{
    place(frame, assets, kFloor, unison::Float3{0.0F, 0.0F, 0.0F}, unison::Quaternion{});

    place(frame, assets, kWall, unison::Float3{0.0F, kWallHeight, -kWallDistance}, unison::Quaternion{});
    place(frame, assets, kWall, unison::Float3{0.0F, kWallHeight, kWallDistance}, unison::Quaternion{});
    place(frame, assets, kWall, unison::Float3{-kWallDistance, kWallHeight, 0.0F}, turnedAboutY(1.5707964F));
    place(frame, assets, kWall, unison::Float3{kWallDistance, kWallHeight, 0.0F}, turnedAboutY(1.5707964F));

    place(frame, assets, kRamp, unison::Float3{-5.0F, 0.9F, 3.0F}, turnedAboutY(0.2F));
    place(frame, assets, kRamp, unison::Float3{5.0F, 0.9F, -3.0F}, turnedAboutY(-0.2F));

    place(frame, assets, kCrate, unison::Float3{-2.0F, 1.0F, 0.0F}, unison::Quaternion{});
    place(frame, assets, kCrate, unison::Float3{2.0F, 1.0F, 1.0F}, unison::Quaternion{});
    place(frame, assets, kCrate, unison::Float3{0.5F, 1.0F, -2.5F}, unison::Quaternion{});
}

void bringInPlayers(unison::sim::Frame& frame, const unison::sim::AssetRegistry& assets, std::size_t players)
{
    const PlayerStats& stats = assets.get<PlayerStats>(kPlayerStats);
    const SpawnPoints& points = assets.get<SpawnPoints>(kSpawnPoints);

    for (std::size_t slot = 0; slot < players; ++slot)
    {
        const entt::entity player = unison::sim::createEntity(frame);

        frame.registry.emplace<unison::sim::Transform>(player, points.positions[slot], unison::Quaternion{});
        frame.registry.emplace<PlayerSlot>(player, static_cast<std::uint8_t>(slot));
        frame.registry.emplace<CharacterState>(player, unison::Float3{}, points.yaws[slot], 0.0F);
        frame.registry.emplace<Health>(player, stats.maxHealth, entt::null);
        frame.registry.emplace<Weapon>(player);
        frame.registry.emplace<Score>(player);

        unison::sim::CharacterController capsule;
        capsule.radius = stats.capsuleRadius;
        capsule.halfHeight = stats.capsuleHalfHeight;

        unison::sim::addCharacter(frame, player, capsule);
    }
}

}

ArenaSimulation::ArenaSimulation(std::size_t players, std::uint16_t tickRate)
    : assetTables{definedArena()}, applyInput{assetTables.get<PlayerStats>(kPlayerStats)},
      characterMove{assetTables.get<PlayerStats>(kPlayerStats)}, weapons{assetTables}, hits{assetTables},
      respawn{assetTables}, matchRules{assetTables}
{
    UNISON_VERIFY(players <= kPlayerCount);
    UNISON_VERIFY(tickRate > 0);

    liveFrame.dt = tickRate > 0 ? 1.0F / static_cast<float>(tickRate) : 0.0F;

    systemPipeline.add(applyInput);
    systemPipeline.add(characterMove);
    systemPipeline.add(weapons);
    systemPipeline.add(physicsStep);
    systemPipeline.add(hits);
    systemPipeline.add(lifetimes);
    systemPipeline.add(respawn);
    systemPipeline.add(matchRules);

    layOutTheArena(liveFrame, assetTables);
    bringInPlayers(liveFrame, assetTables, players > kPlayerCount ? kPlayerCount : players);

    liveFrame.events.clear();
}

void ArenaSimulation::advance(const unison::sim::FrameInputs& inputs)
{
    unison::sim::advanceFrame(liveFrame, systemPipeline, inputs);
}

unison::sim::Frame& ArenaSimulation::frame()
{
    return liveFrame;
}

const unison::sim::Frame& ArenaSimulation::frame() const
{
    return liveFrame;
}

const unison::sim::AssetRegistry& ArenaSimulation::assets() const
{
    return assetTables;
}

const unison::sim::SystemPipeline& ArenaSimulation::pipeline() const
{
    return systemPipeline;
}

}
