#pragma once

#include <cstdint>

namespace unison::sim
{

/// The group a body belongs to. Two static bodies can never meet, which is what lets the broad
/// phase skip every pair that will not move.
enum class PhysicsLayer : std::uint16_t
{
    Static,
    Moving
};

inline constexpr std::uint32_t kPhysicsLayerCount = 2;

/// Whether two groups are allowed to produce a contact.
[[nodiscard]] constexpr bool layersCollide(PhysicsLayer first, PhysicsLayer second)
{
    return first == PhysicsLayer::Moving || second == PhysicsLayer::Moving;
}

}
