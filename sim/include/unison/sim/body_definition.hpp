#pragma once

#include <unison/core/float3.hpp>
#include <unison/sim/physics_layer.hpp>

#include <cstdint>

namespace unison::sim
{

/// The form a body's collision shape takes, which decides the measurements its definition is read for.
enum class BodyShape : std::uint8_t
{
    Box,
    Sphere,
    Capsule
};

/// How a body is allowed to move: not at all, under the forces of the world, or only where the game
/// puts it.
enum class BodyMotion : std::uint8_t
{
    Static,
    Dynamic,
    Kinematic
};

/// The design data a physics body is built from: its measurements in metres, how it may move, the
/// group it collides in, the surface it presents to the bodies it meets, and what it weighs in
/// kilogrammes, which Jolt works out from the shape when it is left at nothing.
struct BodyDefinition
{
    Float3 halfExtents{0.5F, 0.5F, 0.5F};
    float radius = 0.5F;
    float halfHeight = 0.5F;
    float friction = 0.2F;
    float restitution = 0.0F;
    float mass = 0.0F;
    BodyShape shape = BodyShape::Box;
    BodyMotion motion = BodyMotion::Static;
    PhysicsLayer layer = PhysicsLayer::Static;
};

}
