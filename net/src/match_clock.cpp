#include <unison/net/match_clock.hpp>

#include <unison/core/contract.hpp>

namespace unison::net
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;

}

MatchClock::MatchClock(std::uint16_t tickRate) : tickRate{tickRate}
{
    UNISON_VERIFY(tickRate > 0);
}

void MatchClock::anchor(std::uint32_t frame, std::uint64_t now)
{
    if (!firstInput.has_value())
    {
        firstInput = Anchor{frame, now};
    }
}

std::uint32_t MatchClock::dueFrameAt(std::uint64_t now) const
{
    if (!firstInput.has_value())
    {
        return 0;
    }

    if (now < firstInput->arrivedAt)
    {
        return firstInput->frame;
    }

    const std::uint64_t ticksSince = (now - firstInput->arrivedAt) * tickRate / kMicrosecondsPerSecond;

    return firstInput->frame + static_cast<std::uint32_t>(ticksSince);
}

}
