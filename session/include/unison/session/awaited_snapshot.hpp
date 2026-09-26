#pragma once

#include <unison/core/error.hpp>
#include <unison/net/protocol.hpp>
#include <unison/session/snapshot_chunks.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <optional>

namespace unison::session
{

/// The snapshot a client welcomed into a running match waits for: the frame its welcome named, and the chunks of it
/// that have come.
class AwaitedSnapshot
{
public:
    explicit AwaitedSnapshot(std::uint32_t frame);

    /// Takes a chunk in and, once the last has come, gives the snapshot they make, or an error when they are of another
    /// frame or make no snapshot the reader takes; nothing while chunks are missing or for a chunk refused.
    [[nodiscard]] std::optional<tl::expected<sim::FrameSnapshot, Error>> take(const net::SnapshotChunk& chunk);

private:
    std::uint32_t frame;
    SnapshotAssembler assembler;
};

}
