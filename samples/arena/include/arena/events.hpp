#pragma once

#include <unison/core/float3.hpp>
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

/// A shot reached a player, and where it reached them.
struct Hit
{
    entt::entity shot = entt::null;
    entt::entity target = entt::null;
    entt::entity firedBy = entt::null;
    unison::Float3 at{};
};

}

UNISON_EVENT(arena::Fired, unison::sim::EventKind::Predicted);
UNISON_EVENT(arena::Hit, unison::sim::EventKind::Predicted);
