#pragma once

#include <unison/core/fixed_vector.hpp>
#include <unison/core/hasher.hpp>

#include <unison/sim/padding_free.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace unison::sim
{

/// What snapshots and checksums need to know about one component type: the name that identifies it
/// across builds, the bytes a pool of it occupies, and the one operation that needs the type back.
struct ComponentInfo
{
    std::string_view name;
    std::size_t size = 0;
    std::size_t alignment = 0;
    void (*clonePool)(const entt::registry& source, entt::registry& destination) = nullptr;
    void (*hashPool)(Hasher& hasher, const entt::registry& registry) = nullptr;
};

/// The ordered list of component types a simulation is built from. The order is the order they were
/// added in and it decides how snapshots and checksums walk the pools, so every component must be
/// registered from one translation unit: across translation units static initialisation order is
/// unspecified, and Debug and Release would disagree about the order without saying so.
class ComponentRegistry
{
public:
    static constexpr std::size_t kMaxComponents = 128;

    void add(const ComponentInfo& component, std::string_view file);

    [[nodiscard]] std::span<const ComponentInfo> components() const;

private:
    [[nodiscard]] bool holds(std::string_view name) const;

    unison::FixedVector<ComponentInfo, kMaxComponents> entries{};
    std::string_view registrationFile;
};

/// Copies every element of one component pool into another registry, keeping the packed order so a
/// clone iterates in exactly the order its source did.
template <typename T>
void clonePoolOf(const entt::registry& source, entt::registry& destination)
{
    const auto* sourcePool = source.storage<T>();

    if (sourcePool == nullptr)
    {
        return;
    }

    auto& destinationPool = destination.storage<T>();
    destinationPool.reserve(sourcePool->size());

    const entt::registry::common_type& sourceEntities = *sourcePool;

    for (auto first = sourceEntities.rbegin(), last = sourceEntities.rend(); first != last; ++first)
    {
        destinationPool.emplace(*first, sourcePool->get(*first));
    }
}

/// Folds one component pool into a hash in packed order, identifiers included, so a checksum covers
/// which entity holds what and not only the values.
template <typename T>
void hashPoolOf(Hasher& hasher, const entt::registry& registry)
{
    const auto* pool = registry.storage<T>();
    const auto count = static_cast<std::uint32_t>(pool == nullptr ? 0 : pool->size());

    hasher.add(count);

    if (pool == nullptr)
    {
        return;
    }

    const entt::registry::common_type& entities = *pool;

    for (auto first = entities.rbegin(), last = entities.rend(); first != last; ++first)
    {
        hasher.add(static_cast<std::uint32_t>(*first));
        hasher.add(pool->get(*first));
    }
}

/// The registry UNISON_COMPONENT writes into, shared by the whole process because a static
/// initialiser has nowhere else to write.
[[nodiscard]] ComponentRegistry& componentRegistry();

/// Whether the game running in this process registered a component of that name. The engine asks
/// before it puts a component of its own on an entity, because one the game forgot to register
/// would quietly stay out of every snapshot.
[[nodiscard]] bool isComponentRegistered(std::string_view name);

/// Adds one component to the process-wide registry as the program starts. Created by
/// UNISON_COMPONENT; there is no reason to create one directly.
class ComponentRegistration
{
public:
    ComponentRegistration(const ComponentInfo& component, std::string_view file);
};

}

/// Registers a component type under the unqualified name written here, so it must be spelled from
/// inside its own namespace. A pool of it is copied and hashed as raw bytes, so it must be plain
/// data, trivially copyable and free of the padding a compiler would leave indeterminate.
#define UNISON_COMPONENT(Type)                                                                                         \
    static_assert(std::is_aggregate_v<Type>, #Type " must be a plain data aggregate to be a component");               \
    static_assert(std::is_trivially_copyable_v<Type>, #Type " must be trivially copyable to be a component");          \
    static_assert(::unison::sim::PaddingFree<Type>, #Type " must have no padding to be a component: reorder fields");  \
    static const ::unison::sim::ComponentRegistration unisonComponentRegistration##Type                                \
    {                                                                                                                  \
        ::unison::sim::ComponentInfo{                                                                                  \
            #Type, sizeof(Type), alignof(Type), &::unison::sim::clonePoolOf<Type>, &::unison::sim::hashPoolOf<Type>},  \
            __FILE__                                                                                                   \
    }
