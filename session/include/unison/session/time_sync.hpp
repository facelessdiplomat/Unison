#pragma once

#include <unison/net/protocol.hpp>

#include <cstdint>
#include <optional>

namespace unison::session
{

/// How a client keeps pace with the relay: how many pongs it averages before it judges where it stands, and
/// how far from where it should be it may drift before it corrects.
struct TimeSyncSettings
{
    std::uint32_t pongsPerJudgement = 4;
    std::uint64_t jitterMarginMicroseconds = 33'333;
};

/// Judges from the relay's pongs whether a client runs where it should: half a round trip ahead of the frame
/// the relay's clock has due, so that its inputs reach the relay as their frames fall due. A client further
/// behind than the jitter margin, the slower one or one back from a hitch the relay's deadline covered for,
/// runs one tick more per host frame until it has caught up, and one further ahead runs one fewer. The relay's
/// clock runs whatever the players do, so nobody slows down for a player who is out.
class TimeSync
{
public:
    /// A match that never ticks breaks a contract, and so do settings that would judge on no pong at all.
    explicit TimeSync(std::uint16_t tickRate, const TimeSyncSettings& settings = TimeSyncSettings{});

    /// Takes in a pong that came back at `now` while the client stood at its predicted frame. A pong from before
    /// the relay's clock started is left out, and so are those that come back while a correction is still being
    /// run or whose pings went out before it had run its course: they judge a client about to move.
    void observe(const net::Pong& pong, std::uint64_t now, std::uint32_t predictedFrame);

    /// The ticks to add to the host frame played at `now`: one fewer, one more, or none.
    [[nodiscard]] std::int32_t takeCorrection(std::uint64_t now);

    /// How long the last pong took to come back, in microseconds; nothing before the first one.
    [[nodiscard]] std::uint64_t roundTripMicroseconds() const;

    /// How far ahead of the relay's clock the client played when the last pong came back, in microseconds: its
    /// predicted frame against the one the pong had due, less the half round trip since. Nothing before the clock
    /// starts; about half a round trip once the client keeps pace.
    [[nodiscard]] std::int64_t leadMicroseconds() const;

private:
    void judge(std::int64_t aheadOfPace);

    std::uint64_t tickMicroseconds;
    TimeSyncSettings settings;
    std::int64_t aheadSum = 0;
    std::uint32_t pongsSeen = 0;
    std::int32_t pendingTicks = 0;
    std::optional<std::uint64_t> lastCorrectedAt;
    std::uint64_t lastRoundTrip = 0;
    std::int64_t lastLead = 0;
};

}
