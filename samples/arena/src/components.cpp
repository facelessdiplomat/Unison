#include <arena/components.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/character.hpp>
#include <unison/sim/component_registry.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

namespace unison::sim
{

UNISON_COMPONENT(Transform);
UNISON_FIELDS(Transform, position, rotation);
UNISON_COMPONENT(BodyDefinition);
UNISON_FIELDS(BodyDefinition, halfExtents, radius, halfHeight, friction, restitution, mass, shape, motion, layer);
UNISON_COMPONENT(PhysicsBody);
UNISON_FIELDS(PhysicsBody, id, definition);
UNISON_COMPONENT(CharacterController);
UNISON_FIELDS(CharacterController, id, velocity, radius, halfHeight, ground);

}

namespace arena
{

UNISON_COMPONENT(PlayerSlot);
UNISON_FIELDS(PlayerSlot, slot);
UNISON_COMPONENT(CharacterState);
UNISON_FIELDS(CharacterState, desiredVelocity, yaw, verticalSpeed);
UNISON_COMPONENT(Health);
UNISON_FIELDS(Health, points, lastHitBy);
UNISON_COMPONENT(Weapon);
UNISON_FIELDS(Weapon, framesUntilReady);
UNISON_COMPONENT(Projectile);
UNISON_FIELDS(Projectile, firedBy, damage, velocity);
UNISON_COMPONENT(Lifetime);
UNISON_FIELDS(Lifetime, framesLeft);
UNISON_COMPONENT(RespawnTimer);
UNISON_FIELDS(RespawnTimer, framesLeft);
UNISON_COMPONENT(Score);
UNISON_FIELDS(Score, kills);
UNISON_COMPONENT(Killed);
UNISON_FIELDS(Killed, by);

}
