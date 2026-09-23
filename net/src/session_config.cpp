#include <unison/net/session_config.hpp>

#include <unison/core/hasher.hpp>

namespace unison::net
{

std::uint64_t hashOf(const SessionConfig& config)
{
    Hasher hasher;

    hasher.add(config.tickRate);
    hasher.add(config.slotCount);
    hasher.add(config.inputSize);
    hasher.add(config.maxPrediction);
    hasher.add(config.checksumInterval);
    hasher.add(config.seed);
    hasher.add(config.assetHash);
    hasher.add(config.pipelineHash);
    hasher.add(config.buildId);

    return hasher.finish();
}

}
