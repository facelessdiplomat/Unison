#include <unison/sim/entity_lifecycle.hpp>

#include <unison/core/contract.hpp>

namespace unison::sim
{

entt::entity createEntity(Frame& frame)
{
    const entt::entity entity = frame.registry.create();

    frame.events.raise(frame.frameNumber, EntityCreated{entity});

    return entity;
}

void destroyEntity(Frame& frame, entt::entity entity)
{
    UNISON_VERIFY(frame.registry.valid(entity));

    frame.events.raise(frame.frameNumber, EntityDestroyed{entity});
    frame.registry.destroy(entity);
}

}
