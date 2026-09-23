#pragma once

#include <cstdint>
#include <optional>

namespace unison::net
{

/// The frames of a match as the relay's clock has them fall due: the newest frame of the first input to
/// arrive falls due as it arrives, and one frame more every tick after it. It is the one pace every client
/// keeps to, free of the jitter that any player's own inputs carry.
class MatchClock
{
public:
    /// A match that never ticks breaks a contract.
    explicit MatchClock(std::uint16_t tickRate);

    /// Starts the clock with the newest frame of the first input to arrive, at `now` in microseconds; the
    /// inputs after it leave the clock where it was.
    void anchor(std::uint32_t frame, std::uint64_t now);

    /// The newest frame due at `now`: nought before the first input arrived, and that input's frame for any
    /// time before it did.
    [[nodiscard]] std::uint32_t dueFrameAt(std::uint64_t now) const;

private:
    struct Anchor
    {
        std::uint32_t frame = 0;
        std::uint64_t arrivedAt = 0;
    };

    std::uint16_t tickRate;
    std::optional<Anchor> firstInput;
};

}
