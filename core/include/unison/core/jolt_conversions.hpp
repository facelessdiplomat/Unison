#pragma once

#include <unison/core/float3.hpp>
#include <unison/core/quaternion.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>

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

}
