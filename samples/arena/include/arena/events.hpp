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

/// A player ran out of health, and who took the last of it.
struct Died
{
    entt::entity player = entt::null;
    entt::entity killedBy = entt::null;
};

/// A player came back into the match.
struct Respawned
{
    entt::entity player = entt::null;
};

}

UNISON_EVENT(arena::Fired, unison::sim::EventKind::Predicted);
UNISON_EVENT(arena::Hit, unison::sim::EventKind::Predicted);
UNISON_EVENT(arena::Died, unison::sim::EventKind::VerifiedOnly);
UNISON_EVENT(arena::Respawned, unison::sim::EventKind::VerifiedOnly);
