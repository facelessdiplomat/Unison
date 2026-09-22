#pragma once

#include <unison/core/asset_id.hpp>
#include <unison/core/float3.hpp>
#include <unison/sim/asset_registry.hpp>

#include <array>
#include <cstdint>

namespace arena
{

/// How many players one match of the arena holds.
inline constexpr std::size_t kPlayerCount = 4;

inline constexpr unison::AssetId kFloor = unison::makeAssetId("arena.floor");
inline constexpr unison::AssetId kWall = unison::makeAssetId("arena.wall");
inline constexpr unison::AssetId kRamp = unison::makeAssetId("arena.ramp");
inline constexpr unison::AssetId kCrate = unison::makeAssetId("arena.crate");
inline constexpr unison::AssetId kSpawnPoints = unison::makeAssetId("arena.spawn_points");
inline constexpr unison::AssetId kPlayerStats = unison::makeAssetId("arena.player_stats");
inline constexpr unison::AssetId kProjectileStats = unison::makeAssetId("arena.projectile_stats");

/// Where players come into the match and which way they face when they arrive.
struct SpawnPoints
{
    std::array<unison::Float3, kPlayerCount> positions{};
    std::array<float, kPlayerCount> yaws{};
};

/// What a player is made of: how fast they move, how high they jump, how hard the match pulls them
/// down, the capsule they take up, and how long it takes them to come back.
struct PlayerStats
{
    float moveSpeed = 6.0F;
    float jumpSpeed = 5.5F;
    float gravity = -18.0F;
    float capsuleRadius = 0.3F;
    float capsuleHalfHeight = 0.6F;
    std::int32_t maxHealth = 100;
    std::uint32_t respawnFrames = 180;
};

/// What a shot is made of: how fast it flies, how big it is, what it takes off a player it reaches,
/// how long it lives if it reaches nothing, and how long a weapon waits before the next one.
struct ProjectileStats
{
    float speed = 30.0F;
    float radius = 0.12F;
    std::int32_t damage = 25;
    std::uint32_t lifetimeFrames = 120;
    std::uint32_t cooldownFrames = 12;
};

/// Fills the registry with everything a match of the arena is built from and freezes it, so the
/// number clients compare before they play covers the whole of it.
void defineArena(unison::sim::AssetRegistry& assets);

}
