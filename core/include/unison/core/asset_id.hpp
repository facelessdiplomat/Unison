#pragma once

#include <cstdint>
#include <string_view>

namespace unison
{

/// Stable 32-bit identifier of an asset, hashed from its name. It is an enum so that it survives as
/// a non-type template argument and as a switch label without decaying to a bare integer, and its
/// value reaches checksums and the wire, so the hash behind it must never change.
enum class AssetId : std::uint32_t
{
};

/// Hashes a name into its AssetId with FNV-1a 32 in its published form, so the same name yields the
/// same id in every build, in every configuration and on every machine.
[[nodiscard]] constexpr AssetId makeAssetId(std::string_view name)
{
    constexpr std::uint32_t kOffsetBasis = 2166136261U;
    constexpr std::uint32_t kPrime = 16777619U;

    std::uint32_t hash = kOffsetBasis;

    for (const char character : name)
    {
        hash ^= static_cast<std::uint32_t>(static_cast<unsigned char>(character));
        hash *= kPrime;
    }

    return static_cast<AssetId>(hash);
}

}
