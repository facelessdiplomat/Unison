#include <unison/sim/registry_serialization.hpp>

#include "registry_storage.hpp"

#include <unison/core/contract.hpp>
#include <unison/sim/component_registry.hpp>

#include <entt/entity/entity.hpp>

#include <cstdint>
#include <optional>

namespace unison::sim
{

namespace
{

bool writeEntities(BinaryWriter& writer, const entt::registry& registry)
{
    const auto* entities = registry.storage<entt::entity>();
    UNISON_VERIFY(entities != nullptr);

    const entt::registry::common_type& identifiers = *entities;
    bool isWritten = writer.writeValue(static_cast<std::uint32_t>(identifiers.size())) &&
                     writer.writeValue(static_cast<std::uint32_t>(entities->free_list()));

    for (auto first = identifiers.rbegin(), last = identifiers.rend(); isWritten && first != last; ++first)
    {
        isWritten = writer.writeValue(static_cast<std::uint32_t>(*first));
    }

    return isWritten;
}

bool readEntities(BinaryReader& reader, entt::registry& registry)
{
    using Traits = entt::entt_traits<entt::entity>;

    const std::optional<std::uint32_t> count = reader.readValue<std::uint32_t>();
    const std::optional<std::uint32_t> alive = count.has_value() ? reader.readValue<std::uint32_t>() : std::nullopt;

    if (!alive.has_value() || *alive > *count || *count > reader.remaining() / sizeof(std::uint32_t))
    {
        return false;
    }

    auto& entities = registry.storage<entt::entity>();
    entities.reserve(*count);
    entt::entity highest{};

    for (std::uint32_t read = 0; read < *count; ++read)
    {
        const auto identifier = static_cast<entt::entity>(reader.readValue<std::uint32_t>().value_or(0));

        if (entities.generate(identifier) != identifier)
        {
            return false;
        }

        highest = identifier > highest ? identifier : highest;
    }

    entities.start_from(Traits::next(highest));
    entities.free_list(*alive);

    return true;
}

}

std::size_t serializedSizeOf(const entt::registry& registry)
{
    const auto* entities = registry.storage<entt::entity>();
    std::size_t size = 2 * sizeof(std::uint32_t) + (entities == nullptr ? 0 : entities->size()) * sizeof(std::uint32_t);

    for (const ComponentInfo& component : componentRegistry().components())
    {
        size += sizeof(std::uint32_t) + component.countPool(registry) * (sizeof(std::uint32_t) + component.size);
    }

    return size;
}

bool writeRegistry(BinaryWriter& writer, const entt::registry& registry)
{
    bool isWritten = writeEntities(writer, registry);

    for (const ComponentInfo& component : componentRegistry().components())
    {
        isWritten = isWritten && component.writePool(writer, registry);
    }

    return isWritten;
}

bool readRegistry(BinaryReader& reader, entt::registry& registry)
{
    emptyStorages(registry);

    bool isRead = readEntities(reader, registry);

    for (const ComponentInfo& component : componentRegistry().components())
    {
        isRead = isRead && component.readPool(reader, registry);
    }

    return isRead;
}

}
