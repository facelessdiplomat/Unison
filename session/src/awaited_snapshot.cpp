#include <unison/session/awaited_snapshot.hpp>

#include <unison/session/snapshot_serializer.hpp>

#include <vector>

namespace unison::session
{

AwaitedSnapshot::AwaitedSnapshot(std::uint32_t frame) : frame{frame}
{
}

std::optional<tl::expected<sim::FrameSnapshot, Error>> AwaitedSnapshot::take(const net::SnapshotChunk& chunk)
{
    if (!assembler.add(chunk) || !assembler.isComplete())
    {
        return std::nullopt;
    }

    if (assembler.frame() != frame)
    {
        return tl::unexpected{
            Error{ErrorCode::MalformedSnapshot, "the snapshot is not of the frame it was awaited at"}};
    }

    const std::vector<std::byte> bytes = assembler.bytes();
    sim::FrameSnapshot snapshot;
    const tl::expected<void, Error> read = deserializeSnapshot(bytes, snapshot);

    if (!read.has_value())
    {
        return tl::unexpected{read.error()};
    }

    return snapshot;
}

}
