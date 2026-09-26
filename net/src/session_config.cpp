#include <unison/net/session_config.hpp>

#include "read_fields.hpp"

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

bool writeSessionConfig(BinaryWriter& writer, const SessionConfig& config)
{
    return writer.writeValue(config.tickRate) && writer.writeValue(config.slotCount) &&
           writer.writeValue(config.inputSize) && writer.writeValue(config.maxPrediction) &&
           writer.writeValue(config.checksumInterval) && writer.writeValue(config.seed) &&
           writer.writeValue(config.assetHash) && writer.writeValue(config.pipelineHash) &&
           writer.writeValue(config.buildId);
}

std::optional<SessionConfig> readSessionConfig(BinaryReader& reader)
{
    SessionConfig config;

    if (!readAll(reader,
                 config.tickRate,
                 config.slotCount,
                 config.inputSize,
                 config.maxPrediction,
                 config.checksumInterval,
                 config.seed,
                 config.assetHash,
                 config.pipelineHash,
                 config.buildId))
    {
        return std::nullopt;
    }

    return config;
}

}
