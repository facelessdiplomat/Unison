#include <unison/session/snapshot_chunks.hpp>

#include <unison/net/message_codec.hpp>

#include <algorithm>

namespace unison::session
{

std::vector<net::SnapshotChunk> chunksOf(std::uint32_t frame, std::span<const std::byte> snapshot)
{
    const std::size_t chunkSize = net::snapshotBytesPerChunk();
    const auto count =
        static_cast<std::uint32_t>(std::max<std::size_t>(1, (snapshot.size() + chunkSize - 1) / chunkSize));
    std::vector<net::SnapshotChunk> chunks;
    chunks.reserve(count);

    for (std::uint32_t index = 0; index < count; ++index)
    {
        const std::size_t begin = std::size_t{index} * chunkSize;
        chunks.push_back(net::SnapshotChunk{
            frame, index, count, snapshot.subspan(begin, std::min(chunkSize, snapshot.size() - begin))});
    }

    return chunks;
}

bool SnapshotAssembler::add(const net::SnapshotChunk& chunk)
{
    if (!snapshotFrame.has_value())
    {
        if (chunk.chunkCount == 0 || chunk.chunkCount > kMostSnapshotChunks)
        {
            return false;
        }

        snapshotFrame = chunk.frame;
        chunks.resize(chunk.chunkCount);
    }

    const bool isOfThisSnapshot = chunk.frame == *snapshotFrame && chunk.chunkCount == chunks.size();

    if (!isOfThisSnapshot || chunk.chunkIndex >= chunks.size() || chunks[chunk.chunkIndex].has_value())
    {
        return false;
    }

    chunks[chunk.chunkIndex] = std::vector<std::byte>{chunk.bytes.begin(), chunk.bytes.end()};
    ++received;

    return true;
}

bool SnapshotAssembler::isComplete() const
{
    return snapshotFrame.has_value() && received == chunks.size();
}

std::optional<std::uint32_t> SnapshotAssembler::frame() const
{
    return snapshotFrame;
}

std::vector<std::byte> SnapshotAssembler::bytes() const
{
    std::vector<std::byte> snapshot;

    if (!isComplete())
    {
        return snapshot;
    }

    for (const std::optional<std::vector<std::byte>>& chunk : chunks)
    {
        snapshot.insert(snapshot.end(), chunk->begin(), chunk->end());
    }

    return snapshot;
}

}
