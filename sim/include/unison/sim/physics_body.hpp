#pragma once

#include <unison/core/asset_id.hpp>
#include <unison/core/body_id.hpp>
#include <unison/sim/asset_registry.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/physics_world.hpp>

#include <entt/entity/entity.hpp>

namespace unison::sim
{

/// The body an entity owns in the physics world, and the definition it was built from so it can be
/// built again after a rollback.
struct PhysicsBody
{
    BodyId id = BodyId::Invalid;
    AssetId definition = AssetId{};
};

/// Gives the entity a body, built from the definition and placed where the entity's transform says.
/// The entity must have a transform and must not already have a body.
void addBody(Frame& frame, PhysicsWorld& world, const AssetRegistry& assets, entt::entity entity, AssetId definition);

/// Takes the entity's body out of the world and its id back to the allocator, so the id can be
/// handed out again. The entity must have a body.
void removeBody(Frame& frame, PhysicsWorld& world, entt::entity entity);

}
