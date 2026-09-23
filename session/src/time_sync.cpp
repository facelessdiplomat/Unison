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

    if (pendingTicks != 0)
    {
        return;
    }

    const std::int64_t framesAhead =
        static_cast<std::int64_t>(predictedFrame) - static_cast<std::int64_t>(pong.confirmedFrame);
    const auto tick = static_cast<std::int64_t>(tickMicroseconds);

    aheadSum += framesAhead * tick - static_cast<std::int64_t>(lastRoundTrip);
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
        pendingTicks = static_cast<std::int32_t>(-ahead / tick);
    }
}

std::int32_t TimeSync::takeCorrection()
{
    if (pendingTicks < 0)
    {
        ++pendingTicks;

        return -1;
    }

    if (pendingTicks > 0)
    {
        --pendingTicks;

        return 1;
    }

    return 0;
}

std::uint64_t TimeSync::roundTripMicroseconds() const
{
    return lastRoundTrip;
}

}
