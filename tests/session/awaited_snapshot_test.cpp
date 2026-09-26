#include <unison/session/awaited_snapshot.hpp>

#include <support/test_components.hpp>
#include <unison/core/error.hpp>
#include <unison/net/message_codec.hpp>
#include <unison/session/snapshot_chunks.hpp>
#include <unison/session/snapshot_serializer.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kFrame = 17;

unison::sim::FrameSnapshot snapshotOfFrame(std::uint32_t frameNumber)
{
    unison::sim::Frame frame;
    frame.frameNumber = frameNumber;
    const entt::entity entity = frame.registry.create();
    frame.registry.emplace<unison::test::Position>(entity, 1.0F, 2.0F);
    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    return snapshot;
}

std::vector<std::byte> serialized(const unison::sim::FrameSnapshot& snapshot)
{
    std::vector<std::byte> bytes;
    unison::session::serializeSnapshot(snapshot, bytes);

    return bytes;
}

}

TEST_CASE("an awaited snapshot is nothing while chunks of it are missing")
{
    const std::vector<std::byte> bytes(2 * unison::net::snapshotBytesPerChunk(), std::byte{1});
    unison::session::AwaitedSnapshot awaited{kFrame};

    const auto taken = awaited.take(unison::session::chunksOf(kFrame, bytes).front());

    REQUIRE_FALSE(taken.has_value());
}

TEST_CASE("an awaited snapshot is the snapshot its chunks make once the last of them has come")
{
    const unison::sim::FrameSnapshot sent = snapshotOfFrame(kFrame);
    const std::vector<std::byte> bytes = serialized(sent);
    unison::session::AwaitedSnapshot awaited{kFrame};

    const auto taken = awaited.take(unison::session::chunksOf(kFrame, bytes).front());

    REQUIRE(taken.has_value());
    REQUIRE(taken->has_value());
    REQUIRE(unison::sim::checksumOf(**taken) == unison::sim::checksumOf(sent));
}

TEST_CASE("chunks of another frame than the awaited one make no snapshot")
{
    const std::vector<std::byte> bytes = serialized(snapshotOfFrame(kFrame + 1));
    unison::session::AwaitedSnapshot awaited{kFrame};

    const auto taken = awaited.take(unison::session::chunksOf(kFrame + 1, bytes).front());

    REQUIRE(taken.has_value());
    REQUIRE_FALSE(taken->has_value());
    REQUIRE(taken->error().code() == unison::ErrorCode::MalformedSnapshot);
}

TEST_CASE("chunks whose bytes the reader refuses make no snapshot")
{
    const std::vector<std::byte> bytes(16, std::byte{7});
    unison::session::AwaitedSnapshot awaited{kFrame};

    const auto taken = awaited.take(unison::session::chunksOf(kFrame, bytes).front());

    REQUIRE(taken.has_value());
    REQUIRE_FALSE(taken->has_value());
    REQUIRE(taken->error().code() == unison::ErrorCode::MalformedSnapshot);
}
