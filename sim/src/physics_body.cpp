#include <unison/sim/physics_body.hpp>

#include <unison/core/contract.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/transform.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace unison::sim
{

namespace
{

std::array<BodyId, kMaxBodies> bodiesTheRegistryNames(const entt::registry& registry)
{
    std::array<BodyId, kMaxBodies> named;
    named.fill(BodyId::Invalid);

    for (const auto [entity, body] : registry.view<const PhysicsBody>().each())
    {
        const std::uint32_t index = bodyIndexOf(body.id);

        UNISON_VERIFY(index < kMaxBodies);

        if (index < kMaxBodies)
        {
            named[index] = body.id;
        }
    }

    return named;
}

}

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

void reconcileBodies(Frame& frame, const AssetRegistry& assets)
{
    const std::array<BodyId, kMaxBodies> named = bodiesTheRegistryNames(frame.registry);

    std::vector<BodyId> held;
    frame.physics.collectBodies(held);

    for (const BodyId id : held)
    {
        if (named[bodyIndexOf(id)] != id)
        {
            frame.physics.destroyBody(id);
        }
    }

    for (const auto [entity, body, placement] : frame.registry.view<const PhysicsBody, const Transform>().each())
    {
        if (!frame.physics.holdsBody(body.id))
        {
            frame.physics.createBody(body.id, assets.get<BodyDefinition>(body.definition), placement);
        }
    }
}

}
