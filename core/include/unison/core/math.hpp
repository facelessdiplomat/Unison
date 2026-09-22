#pragma once

#include <unison/core/contract.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Math/Math.h>
#include <Jolt/Math/Trigonometry.h>

namespace unison::math
{

/// Sine of an angle in radians, from Jolt's polynomial rather than the CRT's, whose result differs
/// between platforms and runtime versions.
[[nodiscard]] inline float sin(float radians)
{
    return JPH::Sin(radians);
}

/// Cosine of an angle in radians, from Jolt's polynomial for the same reason as sin.
[[nodiscard]] inline float cos(float radians)
{
    return JPH::Cos(radians);
}

/// Angle in radians from the positive x axis to the point at the given y and x, over all four
/// quadrants. The argument order is y then x, matching every other atan2.
[[nodiscard]] inline float atan2(float y, float x)
{
    return JPH::ATan2(y, x);
}

/// Square root, correctly rounded by the hardware instruction and therefore identical everywhere.
[[nodiscard]] inline float sqrt(float value)
{
    return JPH::Sqrt(value);
}

/// Value pulled back inside the closed range, left alone when it is already inside. A range whose
/// minimum exceeds its maximum is a contract violation.
[[nodiscard]] constexpr float clamp(float value, float minimum, float maximum)
{
    UNISON_ASSERT(minimum <= maximum);

    return JPH::Clamp(value, minimum, maximum);
}

/// Linear interpolation spelled with a single multiply, so no compiler can fuse it into an FMA whose
/// extra precision would differ from a plain multiply followed by an add.
[[nodiscard]] constexpr float lerp(float from, float to, float fraction)
{
    return from + fraction * (to - from);
}

}
