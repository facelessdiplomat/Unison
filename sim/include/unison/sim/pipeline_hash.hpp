#pragma once

#include <unison/sim/system_pipeline.hpp>

#include <cstdint>

namespace unison::sim
{

/// Folds the names of a pipeline's systems, in order, into the number two clients compare before
/// they agree to play: different rules, or the same rules in another order, means a different game.
[[nodiscard]] std::uint64_t hashOf(const SystemPipeline& pipeline);

}
