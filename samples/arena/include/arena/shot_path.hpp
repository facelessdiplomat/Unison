#pragma once

#include <unison/core/float3.hpp>

namespace arena
{

/// Where along its path a shot came closest to something, and whether that was close enough to
/// count as touching it. The fraction runs from nought at the start of the path to one at its end.
struct ShotApproach
{
    float fraction = 1.0F;
    bool touched = false;
};

/// Sweeps a shot of that radius from one point to another past an upright capsule, and says where
/// along the way the two first came within reach of each other. Both are treated as segments with
/// a thickness, so a shot cannot pass through a body between one tick and the next.
[[nodiscard]] ShotApproach sweepPastCapsule(const unison::Float3& from,
                                            const unison::Float3& to,
                                            float shotRadius,
                                            const unison::Float3& capsuleBottom,
                                            const unison::Float3& capsuleTop,
                                            float capsuleRadius);

}
