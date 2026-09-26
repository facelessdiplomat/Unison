#include <unison/session/snapshot_chunks.hpp>

#include <unison/net/message_codec.hpp>
#include <unison/net/protocol.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kFrame = 300;

std::vector<std::byte> snapshotOf(std::size_t size)
{
    std::vector<std::byte> bytes(size);

    for (std::size_t index = 0; index < size; ++index)
    {
        bytes[index] = static_cast<std::byte>(index * 7U);
    }

    return bytes;
}

}

TEST_CASE("a snapshot split into chunks reassembles to its bytes whatever order the chunks arrive in")
{
    const std::vector<std::byte> snapshot = snapshotOf(3 * unison::net::snapshotBytesPerChunk() - 5U);
    std::vector<unison::net::SnapshotChunk> chunks = unison::session::chunksOf(kFrame, snapshot);
    std::ranges::reverse(chunks);
    unison::session::SnapshotAssembler assembler;

    for (const unison::net::SnapshotChunk& chunk : chunks)
    {
        REQUIRE(assembler.add(chunk));
    }

    REQUIRE(chunks.size() == 3U);
    REQUIRE(assembler.isComplete());
    REQUIRE(assembler.frame() == kFrame);
    REQUIRE(assembler.bytes() == snapshot);
}

TEST_CASE("an assembler refuses a chunk of another frame or count and one it has already")
{
    const std::vector<std::byte> snapshot = snapshotOf(2 * unison::net::snapshotBytesPerChunk());
    const std::vector<unison::net::SnapshotChunk> chunks = unison::session::chunksOf(kFrame, snapshot);
    unison::session::SnapshotAssembler assembler;
    REQUIRE(assembler.add(chunks.front()));
    unison::net::SnapshotChunk otherFrame = chunks.back();
    otherFrame.frame = kFrame + 1U;
    unison::net::SnapshotChunk otherCount = chunks.back();
    otherCount.chunkCount = 3;

    const bool isOtherFrameAdded = assembler.add(otherFrame);
    const bool isOtherCountAdded = assembler.add(otherCount);
    const bool isRepeatAdded = assembler.add(chunks.front());

    REQUIRE_FALSE(isOtherFrameAdded);
    REQUIRE_FALSE(isOtherCountAdded);
    REQUIRE_FALSE(isRepeatAdded);
    REQUIRE_FALSE(assembler.isComplete());
}

TEST_CASE("an assembler refuses a snapshot of more chunks than it holds")
{
    const std::vector<std::byte> bytes(1);
    unison::session::SnapshotAssembler assembler;

    REQUIRE_FALSE(
        assembler.add(unison::net::SnapshotChunk{kFrame, 0, unison::session::kMostSnapshotChunks + 1U, bytes}));
}
