#include <unison/session/snapshot_serializer.hpp>

#include <support/test_components.hpp>
#include <unison/core/error.hpp>
#include <unison/core/rng.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_snapshot.hpp>
#include <unison/sim/globals.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace
{

constexpr std::size_t kVersionOffset = 4;
constexpr std::size_t kLayoutOffset = 6;
constexpr std::uint32_t kFrameNumber = 17;
constexpr float kStep = 1.0F / 60.0F;

unison::sim::FrameSnapshot populatedSnapshot()
{
    unison::sim::Frame frame;
    frame.frameNumber = kFrameNumber;
    frame.dt = kStep;
    frame.globals.rng = unison::Rng{42};
    frame.globals.matchPhase = unison::sim::MatchPhase::Playing;
    const unison::BodyId released = frame.globals.bodyIds.allocate();
    static_cast<void>(frame.globals.bodyIds.allocate());
    frame.globals.bodyIds.release(released);
    const entt::entity first = frame.registry.create();
    const entt::entity second = frame.registry.create();
    frame.registry.emplace<unison::test::Health>(first, 10);
    frame.registry.emplace<unison::test::Position>(second, 1.0F, 2.0F);
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

tl::expected<void, unison::Error> readBack(std::span<const std::byte> bytes)
{
    unison::sim::FrameSnapshot snapshot;

    return unison::session::deserializeSnapshot(bytes, snapshot);
}

}

TEST_CASE("a snapshot read back from its bytes checksums as the one written")
{
    const unison::sim::FrameSnapshot written = populatedSnapshot();
    unison::sim::FrameSnapshot read;

    const tl::expected<void, unison::Error> isRead = unison::session::deserializeSnapshot(serialized(written), read);

    REQUIRE(isRead.has_value());
    REQUIRE(read.frameNumber == kFrameNumber);
    REQUIRE(read.dt == kStep);
    REQUIRE(unison::sim::checksumOf(read) == unison::sim::checksumOf(written));
}

TEST_CASE("bytes that do not open with the snapshot magic are refused")
{
    std::vector<std::byte> bytes = serialized(populatedSnapshot());
    bytes[0] ^= std::byte{0xFF};

    const tl::expected<void, unison::Error> isRead = readBack(bytes);

    REQUIRE_FALSE(isRead.has_value());
    REQUIRE(isRead.error().code() == unison::ErrorCode::MalformedSnapshot);
}

TEST_CASE("a snapshot of another version of the format is refused")
{
    std::vector<std::byte> bytes = serialized(populatedSnapshot());
    bytes[kVersionOffset] ^= std::byte{0x01};

    const tl::expected<void, unison::Error> isRead = readBack(bytes);

    REQUIRE_FALSE(isRead.has_value());
    REQUIRE(isRead.error().code() == unison::ErrorCode::MalformedSnapshot);
}

TEST_CASE("a snapshot of another component layout is refused")
{
    std::vector<std::byte> bytes = serialized(populatedSnapshot());
    bytes[kLayoutOffset] ^= std::byte{0x01};

    const tl::expected<void, unison::Error> isRead = readBack(bytes);

    REQUIRE_FALSE(isRead.has_value());
    REQUIRE(isRead.error().code() == unison::ErrorCode::MalformedSnapshot);
}

TEST_CASE("a snapshot cut short anywhere is refused")
{
    const std::vector<std::byte> bytes = serialized(populatedSnapshot());
    std::size_t refused = 0;

    for (std::size_t length = 0; length < bytes.size(); ++length)
    {
        refused += readBack(std::span{bytes}.first(length)).has_value() ? 0U : 1U;
    }

    REQUIRE(refused == bytes.size());
}

TEST_CASE("the reader answers every snapshot with one byte corrupted, refusing some")
{
    const std::vector<std::byte> bytes = serialized(populatedSnapshot());
    std::size_t refused = 0;

    for (std::size_t corrupted = 0; corrupted < bytes.size(); ++corrupted)
    {
        std::vector<std::byte> altered = bytes;
        altered[corrupted] ^= std::byte{0xFF};
        refused += readBack(altered).has_value() ? 0U : 1U;
    }

    REQUIRE(refused > 0U);
}
