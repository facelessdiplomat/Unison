#include <unison/sim/frame_checksum.hpp>

#include <unison/core/contract.hpp>
#include <unison/core/hasher.hpp>
#include <unison/sim/component_registry.hpp>

#include <entt/entity/entity.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unison::sim
{

namespace
{

void hashGlobals(Hasher& hasher, const Globals& globals)
{
    hasher.add(globals.rng);
    hasher.add(globals.matchPhase);
    hasher.add(std::as_bytes(globals.bodyIds.freeIds()));
    hasher.add(globals.bodyIds.takenSlotCount());
}

void hashPhysics(Hasher& hasher, const PhysicsWorld& physics)
{
    std::vector<std::byte> state;

    physics.saveState(state);

    hasher.add(std::span<const std::byte>{state});
}

void hashIdentifiers(Hasher& hasher, const entt::registry& registry)
{
    const auto* entities = registry.storage<entt::entity>();
    UNISON_VERIFY(entities != nullptr);

    const entt::registry::common_type& identifiers = *entities;

    hasher.add(static_cast<std::uint32_t>(identifiers.size()));
    hasher.add(static_cast<std::uint32_t>(entities->free_list()));

    for (auto first = identifiers.rbegin(), last = identifiers.rend(); first != last; ++first)
    {
        hasher.add(static_cast<std::uint32_t>(*first));
    }
}

}

std::uint64_t checksumOf(const Frame& frame)
{
    Hasher hasher;

    hashGlobals(hasher, frame.globals);
    hashIdentifiers(hasher, frame.registry);

    for (const ComponentInfo& component : componentRegistry().components())
    {
        component.hashPool(hasher, frame.registry);
    }

    hashPhysics(hasher, frame.physics);

    return hasher.finish();
}

}
