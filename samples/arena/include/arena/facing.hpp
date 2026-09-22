#pragma once

#include <unison/core/float3.hpp>
#include <unison/core/math.hpp>

namespace arena
{

/// Which way a character with that yaw is looking: a yaw of nothing looks along positive Z and the
/// turn goes about the up axis, which is the one convention the whole game reads a yaw by.
[[nodiscard]] inline unison::Float3 facingOf(float yaw)
{
    return unison::Float3{unison::math::sin(yaw), 0.0F, unison::math::cos(yaw)};
}

/// Which way is to the right of a character with that yaw.
[[nodiscard]] inline unison::Float3 rightOf(float yaw)
{
    return unison::Float3{unison::math::cos(yaw), 0.0F, -unison::math::sin(yaw)};
}

}
