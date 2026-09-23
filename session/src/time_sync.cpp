#include <unison/session/time_sync.hpp>

#include <unison/core/contract.hpp>

#include <cstdlib>

namespace unison::session
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;

std::uint64_t microsecondsPerTick(std::uint16_t tickRate)
{
    UNISON_VERIFY(tickRate > 0);

    return tickRate > 0 ? kMicrosecondsPerSecond / tickRate : kMicrosecondsPerSecond;
}

}

TimeSync::TimeSync(std::uint16_t tickRate, const TimeSyncSettings& settings)
    : tickMicroseconds{microsecondsPerTick(tickRate)}, settings{settings}
{
    UNISON_VERIFY(settings.pongsPerJudgement > 0);
}

void TimeSync::observe(const net::Pong& pong, std::uint64_t now, std::uint32_t predictedFrame)
{
    if (now < pong.pingSentAt)
    {
        return;
    }

    lastRoundTrip = now - pong.pingSentAt;

    const bool isClockStarted = pong.dueFrame != 0;

    if (!isClockStarted)
    {
        return;
    }

    const std::int64_t framesAhead =
        static_cast<std::int64_t>(predictedFrame) - static_cast<std::int64_t>(pong.dueFrame);
    const std::int64_t playedAhead = framesAhead * static_cast<std::int64_t>(tickMicroseconds);
    const auto roundTrip = static_cast<std::int64_t>(lastRoundTrip);

    lastLead = playedAhead - roundTrip / 2;

    const bool isPingedBeforeSettling = lastCorrectedAt.has_value() && pong.pingSentAt <= *lastCorrectedAt;

    if (pendingTicks == 0 && !isPingedBeforeSettling)
    {
        judge(playedAhead - roundTrip);
    }
}

void TimeSync::judge(std::int64_t aheadOfPace)
{
    aheadSum += aheadOfPace;
    ++pongsSeen;

    if (pongsSeen < settings.pongsPerJudgement)
    {
        return;
    }

    const std::int64_t ahead = aheadSum / static_cast<std::int64_t>(pongsSeen);

    aheadSum = 0;
    pongsSeen = 0;

    if (std::llabs(ahead) > static_cast<std::int64_t>(settings.jitterMarginMicroseconds))
    {
        pendingTicks = static_cast<std::int32_t>(-ahead / static_cast<std::int64_t>(tickMicroseconds));
    }
}

std::int32_t TimeSync::takeCorrection(std::uint64_t now)
{
    if (pendingTicks == 0)
    {
        return 0;
    }

    const std::int32_t step = pendingTicks < 0 ? -1 : 1;
    pendingTicks -= step;

    if (pendingTicks == 0)
    {
        lastCorrectedAt = now;
    }

    return step;
}

std::uint64_t TimeSync::roundTripMicroseconds() const
{
    return lastRoundTrip;
}

std::int64_t TimeSync::leadMicroseconds() const
{
    return lastLead;
}

}
