#pragma once

#include <unison/core/float3.hpp>
#include <unison/core/quaternion.hpp>

namespace unison::sim
{

/// Where an entity stands and how it is turned, in metres about a Y-up right-handed frame.
struct Transform
{
    Float3 position{};
    Quaternion rotation{};
};

}
