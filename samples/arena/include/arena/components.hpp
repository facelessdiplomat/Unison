#pragma once

#include <unison/core/float3.hpp>

#include <entt/entity/entity.hpp>

#include <cstdint>

namespace arena
{

/// Which player a character belongs to, named by the slot its input arrives in.
struct PlayerSlot
{
    std::uint8_t slot = 0;
};

/// What a player's character is trying to do this tick: where it wants to go, which way it is
/// facing, and how fast it is moving up or down while it gets there.
struct CharacterState
{
    unison::Float3 desiredVelocity{};
    float yaw = 0.0F;
    float verticalSpeed = 0.0F;
};

/// How much damage a character can still take before it dies, and who last took some off them, so
/// that a death can name the player who caused it.
struct Health
{
    std::int32_t points = 100;
    entt::entity lastHitBy = entt::null;
};

/// The weapon a player carries and how long until it can fire again.
struct Weapon
{
    std::uint32_t framesUntilReady = 0;
};

/// A shot in flight: who fired it, what it does to whatever it reaches, and how fast it is going.
struct Projectile
{
    entt::entity firedBy = entt::null;
    std::int32_t damage = 0;
    unison::Float3 velocity{};
};

/// How many frames an entity has left before it leaves the world.
struct Lifetime
{
    std::uint32_t framesLeft = 0;
};

/// How many frames until a dead player is put back into the match.
struct RespawnTimer
{
    std::uint32_t framesLeft = 0;
};

}
