#include <unison/session/desync_dumper.hpp>

#include <support/test_components.hpp>
#include <unison/core/error.hpp>
#include <unison/session/file_bytes.hpp>
#include <unison/session/snapshot_serializer.hpp>
#include <unison/session/verified_frame_receiver.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{

constexpr std::uint32_t kFrame = 12;
constexpr std::uint8_t kSlot = 3;
constexpr std::uint64_t kChecksum = 0x1234;

std::filesystem::path emptiedScratchDirectory(std::string_view name)
{
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / name;
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);

    return directory;
}

unison::sim::FrameSnapshot snapshotWithHealth(std::int32_t points)
{
    unison::sim::Frame frame;
    frame.registry.emplace<unison::test::Health>(frame.registry.create(), points);
    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);

    return snapshot;
}

}

TEST_CASE("a desync dumper writes the snapshot of a checksummed frame into a file named for the frame and the slot")
{
    const std::filesystem::path directory = emptiedScratchDirectory("unison_desync_dumper_writes");
    unison::session::DesyncDumper dumper{directory};
    const unison::sim::FrameInputs inputs;
    const unison::sim::FrameSnapshot snapshot = snapshotWithHealth(7);
    dumper.frameVerified(unison::session::VerifiedFrame{kFrame, inputs, kChecksum, snapshot});

    const tl::expected<std::filesystem::path, unison::Error> dumped = dumper.dump(kFrame, kSlot);

    REQUIRE(dumped.has_value());
    REQUIRE(dumped->filename() == "desync_12_3.snapshot");
    const tl::expected<std::vector<std::byte>, unison::Error> bytes = unison::session::readFileBytes(*dumped);
    REQUIRE(bytes.has_value());
    unison::sim::FrameSnapshot read;
    REQUIRE(unison::session::deserializeSnapshot(*bytes, read).has_value());
    REQUIRE(unison::sim::checksumOf(read) == unison::sim::checksumOf(snapshot));
}

TEST_CASE("a desync dumper keeps no frame the session took no checksum of")
{
    unison::session::DesyncDumper dumper{emptiedScratchDirectory("unison_desync_dumper_unchecked")};
    const unison::sim::FrameInputs inputs;
    const unison::sim::FrameSnapshot snapshot = snapshotWithHealth(7);
    dumper.frameVerified(unison::session::VerifiedFrame{kFrame, inputs, std::nullopt, snapshot});

    const tl::expected<std::filesystem::path, unison::Error> dumped = dumper.dump(kFrame, kSlot);

    REQUIRE_FALSE(dumped.has_value());
    REQUIRE(dumped.error().code() == unison::ErrorCode::FrameNotKept);
}

TEST_CASE("a desync dumper lets go of frames older than it keeps")
{
    unison::session::DesyncDumper dumper{emptiedScratchDirectory("unison_desync_dumper_lets_go")};
    const unison::sim::FrameInputs inputs;
    const unison::sim::FrameSnapshot snapshot = snapshotWithHealth(7);
    const std::uint32_t muchLater = kFrame + unison::session::kDumpedFramesKept + 1U;
    dumper.frameVerified(unison::session::VerifiedFrame{kFrame, inputs, kChecksum, snapshot});
    dumper.frameVerified(unison::session::VerifiedFrame{muchLater, inputs, kChecksum, snapshot});

    const tl::expected<std::filesystem::path, unison::Error> tooOld = dumper.dump(kFrame, kSlot);
    const tl::expected<std::filesystem::path, unison::Error> recent = dumper.dump(muchLater, kSlot);

    REQUIRE_FALSE(tooOld.has_value());
    REQUIRE(tooOld.error().code() == unison::ErrorCode::FrameNotKept);
    REQUIRE(recent.has_value());
}
