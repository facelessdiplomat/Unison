#pragma once

#include <unison/runner/client_outcome.hpp>

#include <cstdint>
#include <span>
#include <string>

namespace unison::runner
{

/// A table of what rollbacks cost the clients of a run: a row per slot, in slot order, with its rollbacks,
/// their rate per second of play at the tick rate, their mean and deepest depth and the ticks it stalled,
/// closed by a row for every client together.
[[nodiscard]] std::string rollbackSummaryOf(std::span<const ClientOutcome> clients, std::uint16_t tickRate);

}
