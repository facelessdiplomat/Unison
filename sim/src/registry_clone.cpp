#include <unison/sim/registry_clone.hpp>

#include <unison/core/contract.hpp>
#include <unison/sim/component_registry.hpp>

#include <entt/entity/entity.hpp>

namespace unison::sim
{

namespace
{

void cloneEntities(const entt::registry& source, entt::registry& destination)
{
    using Traits = entt::entt_traits<entt::entity>;

    const auto* sourceEntities = source.storage<entt::entity>();
    UNISON_VERIFY(sourceEntities != nullptr);

    auto& destinationEntities = destination.storage<entt::entity>();
    destinationEntities.reserve(sourceEntities->size());

    entt::entity highest{};

    const entt::registry::common_type& identifiers = *sourceEntities;

    for (auto first = identifiers.rbegin(), last = identifiers.rend(); first != last; ++first)
    {
        destinationEntities.generate(*first);
        highest = (*first > highest) ? *first : highest;
    }

    destinationEntities.start_from(Traits::next(highest));
    destinationEntities.free_list(sourceEntities->free_list());
}

}

void cloneRegistry(const entt::registry& source, entt::registry& destination)
{
    UNISON_VERIFY(destination.storage<entt::entity>().size() == 0);

    cloneEntities(source, destination);

    for (const ComponentInfo& component : componentRegistry().components())
    {
        component.clonePool(source, destination);
    }
}

}
