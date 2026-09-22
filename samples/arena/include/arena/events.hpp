#pragma once

#include <unison/sim/event_buffer.hpp>

#include <entt/entity/entity.hpp>

namespace arena
{

/// A player pulled the trigger and a shot left their weapon.
struct Fired
{
    entt::entity player = entt::null;
    entt::entity shot = entt::null;
};

}

UNISON_EVENT(arena::Fired, unison::sim::EventKind::Predicted);
