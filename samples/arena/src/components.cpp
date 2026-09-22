#include <arena/components.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/character.hpp>
#include <unison/sim/component_registry.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

namespace unison::sim
{

UNISON_COMPONENT(Transform);
UNISON_COMPONENT(BodyDefinition);
UNISON_COMPONENT(PhysicsBody);
UNISON_COMPONENT(CharacterController);

}

namespace arena
{

UNISON_COMPONENT(PlayerSlot);
UNISON_COMPONENT(CharacterState);
UNISON_COMPONENT(Health);
UNISON_COMPONENT(Weapon);
UNISON_COMPONENT(Projectile);
UNISON_COMPONENT(Lifetime);
UNISON_COMPONENT(RespawnTimer);

}
