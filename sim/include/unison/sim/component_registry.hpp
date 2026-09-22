#pragma once

#include <unison/core/fixed_vector.hpp>

#include <unison/sim/padding_free.hpp>

#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace unison::sim
{

/// What snapshots and checksums need to know about one component type. The name identifies it in
/// reports and across builds; size and alignment describe the bytes a pool of it occupies.
struct ComponentInfo
{
    std::string_view name;
    std::size_t size = 0;
    std::size_t alignment = 0;
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

/// The registry UNISON_COMPONENT writes into, shared by the whole process because a static
/// initialiser has nowhere else to write.
[[nodiscard]] ComponentRegistry& componentRegistry();

/// Adds one component to the process-wide registry as the program starts. Created by
/// UNISON_COMPONENT; there is no reason to create one directly.
class ComponentRegistration
{
public:
    ComponentRegistration(const ComponentInfo& component, std::string_view file);
};

}

/// Registers a component type so snapshots and checksums know about it. A pool of it is copied and
/// hashed as raw bytes, so it must be plain data, trivially copyable and free of padding the
/// compiler would leave indeterminate.
#define UNISON_COMPONENT(Type)                                                                                         \
    static_assert(std::is_aggregate_v<Type>, #Type " must be a plain data aggregate to be a component");               \
    static_assert(std::is_trivially_copyable_v<Type>, #Type " must be trivially copyable to be a component");          \
    static_assert(::unison::sim::PaddingFree<Type>, #Type " must have no padding to be a component: reorder fields");  \
    static const ::unison::sim::ComponentRegistration unisonComponentRegistration##Type                                \
    {                                                                                                                  \
        ::unison::sim::ComponentInfo{#Type, sizeof(Type), alignof(Type)}, __FILE__                                     \
    }
