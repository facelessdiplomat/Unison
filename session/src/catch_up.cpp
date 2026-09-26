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

}
