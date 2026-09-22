#pragma once

#include <unison/core/body_id.hpp>
#include <unison/core/float3.hpp>

#include <cstdint>

namespace unison::sim
{

/// What a character has under its feet: solid ground it can walk on, a slope too steep to climb,
/// something it touches but cannot stand on, or nothing at all.
enum class GroundState : std::uint32_t
{
    OnGround,
    OnSteepGround,
    NotSupported,
    InAir
};

/// The character an entity walks around as: the upright capsule it takes up, how fast it is moving
/// and what it is standing on. A character lives outside the state Jolt saves for its bodies, so all
/// of this travels in the frame and goes back onto the character when a snapshot is restored.
struct CharacterController
{
    BodyId id = BodyId::Invalid;
    Float3 velocity{};
    float radius = 0.3F;
    float halfHeight = 0.6F;
    GroundState ground = GroundState::InAir;
};

}
