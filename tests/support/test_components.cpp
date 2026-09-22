#include <support/test_components.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/character.hpp>
#include <unison/sim/component_registry.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

namespace unison::test
{

UNISON_COMPONENT(Position);
UNISON_COMPONENT(Health);

}

namespace unison::sim
{

UNISON_COMPONENT(Transform);
UNISON_COMPONENT(BodyDefinition);
UNISON_COMPONENT(CharacterController);
UNISON_COMPONENT(PhysicsBody);

}
