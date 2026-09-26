#pragma once

#include <unison/core/binary_reader.hpp>
#include <unison/core/binary_writer.hpp>

#include <cstdint>
#include <optional>

namespace unison::net
{

/// Everything the clients of one match must agree on before they play it: how fast it ticks, how many
/// players it has and how big their inputs are, how far a client may run ahead, every how many verified
/// frames they compare checksums, where its randomness starts, which assets and rules it runs, and which
/// build of the engine runs them.
struct SessionConfig
{
    std::uint16_t tickRate = 60;
    std::uint8_t slotCount = 0;
    std::uint8_t inputSize = 0;
    std::uint32_t maxPrediction = 20;
    std::uint32_t checksumInterval = 20;
    std::uint64_t seed = 0;
    std::uint64_t assetHash = 0;
    std::uint64_t pipelineHash = 0;
    std::uint64_t buildId = 0;
};

/// Folds every field of a config into the number a client presents when it joins, so a client that
/// disagrees on any one of them is turned away.
[[nodiscard]] std::uint64_t hashOf(const SessionConfig& config);

/// Writes every field of a config little-endian in the order they are declared, as the protocol and a replay
/// carry it. Returns false, part of it written, when the buffer runs out first.
[[nodiscard]] bool writeSessionConfig(BinaryWriter& writer, const SessionConfig& config);

/// Reads a config back as `writeSessionConfig` wrote it, or nothing when the bytes end first.
[[nodiscard]] std::optional<SessionConfig> readSessionConfig(BinaryReader& reader);

}
