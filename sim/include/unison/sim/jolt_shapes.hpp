#pragma once

#include <unison/sim/body_definition.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace unison::sim
{

/// Builds the shape a body definition describes from its measurements; measurements Jolt cannot build a
/// shape from break a contract.
[[nodiscard]] JPH::Ref<JPH::Shape> shapeOf(const BodyDefinition& definition);

/// Builds a sphere of the given radius around its centre.
[[nodiscard]] JPH::Ref<JPH::Shape> sphereShape(float radius);

/// Builds an upright capsule around its centre: a cylinder twice the half height tall, capped by half
/// spheres of the radius.
[[nodiscard]] JPH::Ref<JPH::Shape> capsuleShape(float halfHeight, float radius);

}
