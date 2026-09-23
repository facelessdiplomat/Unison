#pragma once

#include <cstdint>

namespace unison::session
{

/// What rollbacks have cost a session so far: how many there were and the deepest, every frame played
/// again because of them, the ticks spent waiting for the relay, and the frames played the first time.
struct RollbackStats
{
    std::uint32_t rollbacks = 0;
    std::uint32_t deepestRollback = 0;
    std::uint64_t resimulatedFrames = 0;
    std::uint32_t stalledTicks = 0;
    std::uint32_t framesPlayed = 0;

    /// Rollbacks per second of play at the given tick rate; none before a frame has been played.
    [[nodiscard]] double rollbacksPerSecond(std::uint16_t tickRate) const;

    /// Frames played again per second of play at the given tick rate; none before a frame has been played.
    [[nodiscard]] double resimulatedFramesPerSecond(std::uint16_t tickRate) const;

    /// Frames a rollback played again on average; none before the first rollback.
    [[nodiscard]] double meanRollbackDepth() const;
};

}
