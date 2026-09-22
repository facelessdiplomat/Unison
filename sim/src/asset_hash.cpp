#include <unison/sim/asset_hash.hpp>

#include <unison/core/contract.hpp>
#include <unison/core/hasher.hpp>

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

namespace unison::sim
{

std::uint64_t hashOf(const AssetRegistry& registry)
{
    UNISON_VERIFY(registry.isFrozen());

    std::vector<AssetEntry> entries;
    registry.collectAssets(entries);

    std::sort(entries.begin(),
              entries.end(),
              [](const AssetEntry& left, const AssetEntry& right) { return left.id < right.id; });

    Hasher hasher;
    hasher.add(static_cast<std::uint32_t>(entries.size()));

    for (const AssetEntry& entry : entries)
    {
        hasher.add(static_cast<std::uint32_t>(entry.id));
        hasher.add(static_cast<std::uint32_t>(entry.size));
        hasher.add(std::span<const std::byte>{entry.bytes, entry.size});
    }

    return hasher.finish();
}

}
