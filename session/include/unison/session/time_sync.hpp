#pragma once

#include <unison/net/protocol.hpp>

#include <cstdint>

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
/// the relay has confirmed by now. The relay confirms a frame once the last input for it arrives, so that is
/// where the slowest client stands; a client further ahead than the jitter margin runs one tick fewer per
/// host frame until it is back, and one further behind, after a hitch the relay's deadline covered for, runs
/// one tick more.
class TimeSync
{
public:
    /// A match that never ticks breaks a contract, and so do settings that would judge on no pong at all.
    explicit TimeSync(std::uint16_t tickRate, const TimeSyncSettings& settings = TimeSyncSettings{});

    /// Takes in a pong that came back at `now` while the client stood at its predicted frame. Pongs that come
    /// back while a correction is still being run are left out, since they judge a client about to move.
    void observe(const net::Pong& pong, std::uint64_t now, std::uint32_t predictedFrame);

    /// The ticks to add to the next host frame: one fewer, one more, or none.
    [[nodiscard]] std::int32_t takeCorrection();

    /// How long the last pong took to come back, in microseconds; nothing before the first one.
    [[nodiscard]] std::uint64_t roundTripMicroseconds() const;

private:
    std::uint64_t tickMicroseconds;
    TimeSyncSettings settings;
    std::int64_t aheadSum = 0;
    std::uint32_t pongsSeen = 0;
    std::int32_t pendingTicks = 0;
    std::uint64_t lastRoundTrip = 0;
};

}
