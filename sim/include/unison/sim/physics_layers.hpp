#pragma once

#include <unison/core/contract.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace unison::sim
{

/// The group a body belongs to. Two static bodies can never meet, which is what lets the broad
/// phase skip every pair that will not move.
enum class PhysicsLayer : JPH::ObjectLayer
{
    Static,
    Moving
};

inline constexpr JPH::uint kPhysicsLayerCount = 2;

/// Whether two groups are allowed to produce a contact.
[[nodiscard]] constexpr bool layersCollide(PhysicsLayer first, PhysicsLayer second)
{
    return first == PhysicsLayer::Moving || second == PhysicsLayer::Moving;
}

/// Narrows a layer to the number Jolt stores on a body.
[[nodiscard]] constexpr JPH::ObjectLayer toObjectLayer(PhysicsLayer layer)
{
    return static_cast<JPH::ObjectLayer>(layer);
}

/// The broad phase tree a layer keeps its bodies in: every layer has one of its own.
[[nodiscard]] constexpr JPH::BroadPhaseLayer toBroadPhaseLayer(PhysicsLayer layer)
{
    return JPH::BroadPhaseLayer{static_cast<JPH::BroadPhaseLayer::Type>(layer)};
}

/// Widens the number Jolt stores on a body back to a layer.
[[nodiscard]] inline PhysicsLayer toPhysicsLayer(JPH::ObjectLayer layer)
{
    UNISON_ASSERT(layer < kPhysicsLayerCount);

    return static_cast<PhysicsLayer>(layer);
}

/// Names the layer whose bodies a broad phase tree holds.
[[nodiscard]] inline PhysicsLayer toPhysicsLayer(JPH::BroadPhaseLayer tree)
{
    UNISON_ASSERT(tree.GetValue() < kPhysicsLayerCount);

    return static_cast<PhysicsLayer>(tree.GetValue());
}

/// Gives every layer a broad phase tree of its own, so a moving body is tested against the static
/// world and against the other movers separately.
class BroadPhaseLayerMapping final : public JPH::BroadPhaseLayerInterface
{
public:
    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override
    {
        return kPhysicsLayerCount;
    }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return toBroadPhaseLayer(toPhysicsLayer(layer));
    }
};

/// Answers the layer rule while Jolt walks the broad phase trees.
class ObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer tree) const override
    {
        return layersCollide(toPhysicsLayer(layer), toPhysicsLayer(tree));
    }
};

/// Answers the layer rule for a pair of bodies the broad phase has brought together.
class ObjectPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const override
    {
        return layersCollide(toPhysicsLayer(first), toPhysicsLayer(second));
    }
};

}
