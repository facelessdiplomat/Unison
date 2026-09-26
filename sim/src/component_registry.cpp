#include <unison/sim/component_registry.hpp>

#include <unison/core/contract.hpp>

namespace unison::sim
{

void ComponentRegistry::add(const ComponentInfo& component, std::string_view file)
{
    UNISON_VERIFY(!entries.isFull());
    UNISON_VERIFY(!holds(component.name));
    UNISON_VERIFY(entries.size() == 0 || registrationFile == file);

    if (entries.isFull() || holds(component.name) || (entries.size() != 0 && registrationFile != file))
    {
        return;
    }

    registrationFile = file;
    entries.pushBack(component);
}

void ComponentRegistry::nameFields(std::string_view component, std::span<const FieldInfo> fields, std::string_view file)
{
    UNISON_VERIFY(file == registrationFile);
    UNISON_VERIFY(holds(component));

    for (ComponentInfo& entry : entries)
    {
        if (entry.name == component && file == registrationFile)
        {
            entry.fields = fields;
        }
    }
}

std::span<const ComponentInfo> ComponentRegistry::components() const
{
    return std::span<const ComponentInfo>{entries.begin(), entries.size()};
}

std::uint64_t ComponentRegistry::layoutHash() const
{
    Hasher hasher;

    for (const ComponentInfo& component : entries)
    {
        hasher.add(std::as_bytes(std::span{component.name}));
        hasher.add(static_cast<std::uint64_t>(component.size));
    }

    return hasher.finish();
}

bool ComponentRegistry::holds(std::string_view name) const
{
    for (const ComponentInfo& component : entries)
    {
        if (component.name == name)
        {
            return true;
        }
    }

    return false;
}

ComponentRegistry& componentRegistry()
{
    static ComponentRegistry registry;

    return registry;
}

bool isComponentRegistered(std::string_view name)
{
    for (const ComponentInfo& component : componentRegistry().components())
    {
        if (component.name == name)
        {
            return true;
        }
    }

    return false;
}

ComponentRegistration::ComponentRegistration(const ComponentInfo& component, std::string_view file)
{
    componentRegistry().add(component, file);
}

FieldNaming::FieldNaming(std::string_view component, std::span<const FieldInfo> fields, std::string_view file)
{
    componentRegistry().nameFields(component, fields, file);
}

}
