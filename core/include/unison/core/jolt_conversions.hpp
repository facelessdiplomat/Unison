#pragma once

#include <unison/core/body_id.hpp>
#include <unison/core/float3.hpp>
#include <unison/core/quaternion.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <cstdint>

namespace unison
{

/// Widens a stored position or direction into the SIMD vector Jolt computes with.
[[nodiscard]] inline JPH::Vec3 toJoltVector(const Float3& value)
{
    return JPH::Vec3{value.x, value.y, value.z};
}

/// Narrows a Jolt vector back into the padding-free form a component stores.
[[nodiscard]] inline Float3 toFloat3(JPH::Vec3Arg value)
{
    return Float3{value.GetX(), value.GetY(), value.GetZ()};
}

/// Widens a stored rotation into the SIMD quaternion Jolt computes with.
[[nodiscard]] inline JPH::Quat toJoltQuaternion(const Quaternion& value)
{
    return JPH::Quat{value.x, value.y, value.z, value.w};
}

/// Narrows a Jolt quaternion back into the padding-free form a component stores.
[[nodiscard]] inline Quaternion toQuaternion(JPH::QuatArg value)
{
    return Quaternion{value.GetX(), value.GetY(), value.GetZ(), value.GetW()};
}

/// Widens a stored handle into the id Jolt knows the body by.
[[nodiscard]] inline JPH::BodyID toJoltBodyId(BodyId id)
{
    return JPH::BodyID{static_cast<std::uint32_t>(id)};
}

/// Narrows Jolt's body id back into the handle the simulation stores.
[[nodiscard]] inline BodyId toBodyId(const JPH::BodyID& id)
{
    return static_cast<BodyId>(id.GetIndexAndSequenceNumber());
}

}
