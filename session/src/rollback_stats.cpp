#include <unison/session/rollback_stats.hpp>

namespace unison::session
{

namespace
{

double perSecondOfPlay(std::uint64_t count, std::uint32_t framesPlayed, std::uint16_t tickRate)
{
    if (framesPlayed == 0)
    {
        return 0.0;
    }

    return static_cast<double>(count) * static_cast<double>(tickRate) / static_cast<double>(framesPlayed);
}

}

double RollbackStats::rollbacksPerSecond(std::uint16_t tickRate) const
{
    return perSecondOfPlay(rollbacks, framesPlayed, tickRate);
}

double RollbackStats::resimulatedFramesPerSecond(std::uint16_t tickRate) const
{
    return perSecondOfPlay(resimulatedFrames, framesPlayed, tickRate);
}

double RollbackStats::meanRollbackDepth() const
{
    if (rollbacks == 0)
    {
        return 0.0;
    }

    return static_cast<double>(resimulatedFrames) / static_cast<double>(rollbacks);
}

}
