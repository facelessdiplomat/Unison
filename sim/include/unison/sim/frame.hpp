#pragma once

#include <unison/sim/event_buffer.hpp>
#include <unison/sim/globals.hpp>
#include <unison/sim/physics_world.hpp>

#include <entt/entity/registry.hpp>

#include <cstdint>

namespace unison::sim
{

/// The whole mutable state of the game at one tick: the entities and their components, the bodies
/// of the physics world, the state that belongs to no entity, and the events raised while the tick
/// runs. It holds state and nothing else; every rule that changes it lives in a system.
struct Frame
{
    explicit Frame(const PhysicsWorldSettings& settings = PhysicsWorldSettings{}) : physics{settings}
    {
    }

    std::uint32_t frameNumber = 0;
    float dt = 0.0F;
    entt::registry registry;
    Globals globals{};
    EventBuffer events{};
    PhysicsWorld physics;
};

}
