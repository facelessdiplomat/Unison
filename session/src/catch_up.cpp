#include <unison/session/catch_up.hpp>

#include <algorithm>

namespace unison::session
{

void CatchUp::hear(std::uint32_t confirmedFrame)
{
    newestConfirmed = std::max(newestConfirmed, confirmedFrame);
}

void CatchUp::start()
{
    isCatchingUp = true;
}

void CatchUp::update(std::uint32_t predictedFrame)
{
    isCatchingUp = isCatchingUp && predictedFrame < newestConfirmed;
}

bool CatchUp::isBehind() const
{
    return isCatchingUp;
}

std::optional<std::int32_t> CatchUp::extraTicks(std::uint32_t predictedFrame) const
{
    if (!isCatchingUp || predictedFrame >= newestConfirmed)
    {
        return std::nullopt;
    }

    return static_cast<std::int32_t>(
        std::min<std::uint32_t>(static_cast<std::uint32_t>(kCatchUpExtraTicks), newestConfirmed - predictedFrame));
}

std::uint32_t CatchUp::newestHeard() const
{
    return newestConfirmed;
}

std::int32_t
spectatorTickCorrection(std::uint32_t newestConfirmed, std::uint32_t delayFrames, std::uint32_t predictedFrame)
{
    const std::uint32_t playableUpTo = newestConfirmed > delayFrames ? newestConfirmed - delayFrames : 0;

    if (playableUpTo <= predictedFrame)
    {
        return -1;
    }

    return static_cast<std::int32_t>(
        std::min<std::uint32_t>(static_cast<std::uint32_t>(kCatchUpExtraTicks), playableUpTo - predictedFrame - 1));
}

}
