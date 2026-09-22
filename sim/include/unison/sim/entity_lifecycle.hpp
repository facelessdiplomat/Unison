#pragma once

#include <unison/sim/event_buffer.hpp>
#include <unison/sim/frame.hpp>

#include <entt/entity/entity.hpp>

namespace unison::sim
{

/// An entity came into the world this tick. The view shows it at once and drops it again if a
/// rollback takes the tick back.
struct EntityCreated
{
    entt::entity entity = entt::null;
};

/// An entity left the world this tick.
struct EntityDestroyed
{
    entt::entity entity = entt::null;
};

/// Creates an entity and tells the view about it. Simulation code creates entities this way rather
/// than through the registry, so nothing comes into the world unannounced.
[[nodiscard]] entt::entity createEntity(Frame& frame);

/// Destroys an entity and tells the view about it. The entity must still be alive.
void destroyEntity(Frame& frame, entt::entity entity);

}

UNISON_EVENT(unison::sim::EntityCreated, unison::sim::EventKind::Predicted);
UNISON_EVENT(unison::sim::EntityDestroyed, unison::sim::EventKind::Predicted);
