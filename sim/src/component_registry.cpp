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

std::span<const ComponentInfo> ComponentRegistry::components() const
{
    return std::span<const ComponentInfo>{entries.begin(), entries.size()};
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

}
