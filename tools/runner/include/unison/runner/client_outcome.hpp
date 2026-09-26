#pragma once

#include <unison/net/protocol.hpp>
#include <unison/session/rollback_stats.hpp>

#include <cstdint>
#include <span>

namespace unison::runner
{

/// How one client of a run fared: the slot the relay gave it, the frame it started from, past nought for a client that
/// joined late, and what rollbacks cost its session.
struct ClientOutcome
{
    std::uint8_t slot = net::kNoSlot;
    std::uint32_t startFrame = 0;
    session::RollbackStats rollbacks;
};

/// The rollbacks of every client together: their counts added up, the deepest the deepest of any.
[[nodiscard]] session::RollbackStats rollbacksOfAll(std::span<const ClientOutcome> clients);

}
