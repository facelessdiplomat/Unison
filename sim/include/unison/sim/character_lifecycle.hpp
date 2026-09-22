#pragma once

#include <unison/sim/character.hpp>
#include <unison/sim/frame.hpp>

#include <entt/entity/entity.hpp>

namespace unison::sim
{

/// Gives the entity a character to walk around as, standing where its transform says. The entity
/// must have a transform and must not already be a character.
void addCharacter(Frame& frame, entt::entity entity, const CharacterController& character = CharacterController{});

/// Takes the entity's character out of the world and its id back, so the id can be handed out again.
void removeCharacter(Frame& frame, entt::entity entity);

/// Makes the world hold exactly the characters the registry names, which is what Jolt needs before
/// the states saved for them can be put back.
void reconcileCharacters(Frame& frame);

}
