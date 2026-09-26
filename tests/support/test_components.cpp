#include <support/test_components.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/character.hpp>
#include <unison/sim/component_registry.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

namespace unison::test
{

UNISON_COMPONENT(Position);
UNISON_FIELDS(Position, x, y);
UNISON_COMPONENT(Health);
UNISON_FIELDS(Health, points);

}

namespace unison::sim
{

UNISON_COMPONENT(Transform);
UNISON_FIELDS(Transform, position, rotation);
UNISON_COMPONENT(BodyDefinition);
UNISON_FIELDS(BodyDefinition, halfExtents, radius, halfHeight, friction, restitution, mass, shape, motion, layer);
UNISON_COMPONENT(CharacterController);
UNISON_FIELDS(CharacterController, id, velocity, radius, halfHeight, ground);
UNISON_COMPONENT(PhysicsBody);
UNISON_FIELDS(PhysicsBody, id, definition);

}
