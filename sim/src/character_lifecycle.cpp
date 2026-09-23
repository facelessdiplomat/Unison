#include <unison/sim/character_lifecycle.hpp>

#include <unison/core/contract.hpp>
#include <unison/sim/component_registry.hpp>
#include <unison/sim/transform.hpp>

#include <array>
#include <cstdint>

namespace unison::sim
{

namespace
{

std::array<BodyId, kMaxBodies> charactersTheRegistryNames(const entt::registry& registry)
{
    std::array<BodyId, kMaxBodies> named;
    named.fill(BodyId::Invalid);

    for (const auto [entity, character] : registry.view<const CharacterController>().each())
    {
        const std::uint32_t index = bodyIndexOf(character.id);

        UNISON_VERIFY(index < kMaxBodies);

        if (index < kMaxBodies)
        {
            named[index] = character.id;
        }
    }

    return named;
}

}

void addCharacter(Frame& frame, entt::entity entity, const CharacterController& character)
{
    UNISON_ASSERT(isComponentRegistered("Transform"));
    UNISON_ASSERT(isComponentRegistered("CharacterController"));

    UNISON_VERIFY(frame.registry.all_of<Transform>(entity));
    UNISON_VERIFY(!frame.registry.all_of<CharacterController>(entity));

    CharacterController placed = character;
    placed.id = frame.globals.bodyIds.allocate();

    frame.physics.characters().create(placed.id, placed, frame.registry.get<Transform>(entity));
    frame.registry.emplace<CharacterController>(entity, placed);
}

void removeCharacter(Frame& frame, entt::entity entity)
{
    UNISON_VERIFY(frame.registry.all_of<CharacterController>(entity));

    const BodyId id = frame.registry.get<CharacterController>(entity).id;

    frame.physics.characters().destroy(id);
    frame.globals.bodyIds.release(id);
    frame.registry.erase<CharacterController>(entity);
}

void reconcileCharacters(Frame& frame)
{
    frame.physics.characters().keepOnly(charactersTheRegistryNames(frame.registry));

    for (const auto [entity, character, placement] :
         frame.registry.view<const CharacterController, const Transform>().each())
    {
        if (!frame.physics.characters().holds(character.id))
        {
            frame.physics.characters().create(character.id, character, placement);
        }
    }
}

}
