#include <unison/sim/physics_body.hpp>

#include <unison/core/contract.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/transform.hpp>

namespace unison::sim
{

void addBody(Frame& frame, const AssetRegistry& assets, entt::entity entity, AssetId definition)
{
    UNISON_VERIFY(frame.registry.all_of<Transform>(entity));
    UNISON_VERIFY(!frame.registry.all_of<PhysicsBody>(entity));

    const BodyId id = frame.globals.bodyIds.allocate();

    frame.physics.createBody(id, assets.get<BodyDefinition>(definition), frame.registry.get<Transform>(entity));

    frame.registry.emplace<PhysicsBody>(entity, id, definition);
}

void removeBody(Frame& frame, entt::entity entity)
{
    UNISON_VERIFY(frame.registry.all_of<PhysicsBody>(entity));

    const BodyId id = frame.registry.get<PhysicsBody>(entity).id;

    frame.physics.destroyBody(id);
    frame.globals.bodyIds.release(id);
    frame.registry.erase<PhysicsBody>(entity);
}

}
