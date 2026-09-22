#include <unison/sim/asset_registry.hpp>

namespace unison::sim
{

void AssetRegistry::freeze()
{
    frozen = true;
}

bool AssetRegistry::isFrozen() const
{
    return frozen;
}

bool AssetRegistry::holds(AssetId id) const
{
    for (const std::unique_ptr<Table>& table : tables)
    {
        if (table->holds(id))
        {
            return true;
        }
    }

    return false;
}

void AssetRegistry::collectAssets(std::vector<AssetEntry>& entries) const
{
    for (const std::unique_ptr<Table>& table : tables)
    {
        table->collect(entries);
    }
}

const AssetRegistry::Table* AssetRegistry::findTable(entt::id_type typeId) const
{
    for (const std::unique_ptr<Table>& table : tables)
    {
        if (table->typeId() == typeId)
        {
            return table.get();
        }
    }

    return nullptr;
}

}
