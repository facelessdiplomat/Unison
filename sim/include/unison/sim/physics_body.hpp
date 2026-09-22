#pragma once

#include <unison/core/asset_id.hpp>
#include <unison/core/body_id.hpp>
#include <unison/sim/asset_registry.hpp>
#include <unison/sim/frame.hpp>

#include <entt/entity/entity.hpp>

namespace unison::sim
{

/// The body an entity owns in the physics world, and the asset it was built from. What the body is
/// made of travels beside it in a `BodyDefinition` component, which is the copy a system changes.
struct PhysicsBody
{
    BodyId id = BodyId::Invalid;
    AssetId definition = AssetId{};
};

/// Gives the entity a body, built from the definition and placed where the entity's transform says.
/// The entity must have a transform and must not already have a body.
void addBody(Frame& frame, const AssetRegistry& assets, entt::entity entity, AssetId definition);

/// Takes the entity's body out of the world and its id back to the allocator, so the id can be
/// handed out again. The entity must have a body.
void removeBody(Frame& frame, entt::entity entity);

/// Makes the world hold exactly the bodies the registry names, destroying the ones it has lost track
/// of, building the missing ones again from their definitions and putting the properties Jolt does
/// not record back on the rest. Jolt can only restore a state into the set of bodies it was saved from.
void reconcileBodies(Frame& frame);

/// Puts what the entity's definition says onto its body: the properties Jolt leaves out of the state
/// buffer it saves. A system changes them through the component and never on the body itself.
void applyBodyProperties(Frame& frame, entt::entity entity);

}
