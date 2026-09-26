#pragma once

#include <unison/net/protocol.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace unison::session
{

/// The most chunks an assembler takes a snapshot in, some five megabytes, far more than a frame of the sample holds.
inline constexpr std::uint32_t kMostSnapshotChunks = 4096;

/// Splits a serialised snapshot of a frame into chunks of as many bytes as one carries, in order; the chunks view the
/// snapshot's bytes, which must outlive them.
[[nodiscard]] std::vector<net::SnapshotChunk> chunksOf(std::uint32_t frame, std::span<const std::byte> snapshot);

/// Gathers the chunks of one serialised snapshot in whatever order they arrive. The first chunk decides the frame and
/// how many chunks there are; a chunk of another frame or count, one already had, and a snapshot of more chunks than
/// `kMostSnapshotChunks` are refused.
class SnapshotAssembler
{
public:
    /// Takes a chunk in, returning false for one refused.
    [[nodiscard]] bool add(const net::SnapshotChunk& chunk);

    [[nodiscard]] bool isComplete() const;

    /// The frame of the snapshot, once a chunk of it has arrived.
    [[nodiscard]] std::optional<std::uint32_t> frame() const;

    /// The snapshot's bytes, chunk after chunk, once every chunk is in; nothing before.
    [[nodiscard]] std::vector<std::byte> bytes() const;

private:
    std::optional<std::uint32_t> snapshotFrame;
    std::vector<std::optional<std::vector<std::byte>>> chunks;
    std::uint32_t received = 0;
};

}
