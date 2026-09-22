#pragma once

#include <unison/sim/asset_registry.hpp>

#include <cstdint>

namespace unison::sim
{

/// Folds a frozen registry into the number clients compare before agreeing they are playing the
/// same game. Assets are hashed in the order of their identifiers rather than the order a game
/// happened to register them, so rearranging the setup code does not turn one build into a
/// stranger. The registry must already be frozen.
[[nodiscard]] std::uint64_t hashOf(const AssetRegistry& registry);

}
