#include <unison/session/replay_file.hpp>

#include <unison/core/error.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{

std::filesystem::path scratchPath(std::string_view name)
{
    return std::filesystem::temp_directory_path() / name;
}

class RemovedAfterwards
{
public:
    explicit RemovedAfterwards(std::filesystem::path path) : path{std::move(path)}
    {
    }

    RemovedAfterwards(const RemovedAfterwards&) = delete;
    RemovedAfterwards& operator=(const RemovedAfterwards&) = delete;
    RemovedAfterwards(RemovedAfterwards&&) = delete;
    RemovedAfterwards& operator=(RemovedAfterwards&&) = delete;

    ~RemovedAfterwards()
    {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }

private:
    std::filesystem::path path;
};

const std::vector<std::byte> kReplayBytes{
    std::byte{0x55}, std::byte{0x4E}, std::byte{0x52}, std::byte{0x50}, std::byte{0x00}, std::byte{0xFF}};

}

TEST_CASE("a replay file reads back as the bytes written into it")
{
    const std::filesystem::path path = scratchPath("unison_replay_file_round_trip.replay");
    const RemovedAfterwards cleanup{path};
    REQUIRE(unison::session::writeReplayFile(path, kReplayBytes).has_value());

    const tl::expected<std::vector<std::byte>, unison::Error> read = unison::session::readReplayFile(path);

    REQUIRE(read.has_value());
    REQUIRE(*read == kReplayBytes);
}

TEST_CASE("a replay file in a folder that does not exist is not written")
{
    const std::filesystem::path path = scratchPath("unison_replay_file_no_such_folder") / "match.replay";

    const tl::expected<void, unison::Error> written = unison::session::writeReplayFile(path, kReplayBytes);

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code() == unison::ErrorCode::FileUnavailable);
}

TEST_CASE("a replay file that does not exist is not read")
{
    const std::filesystem::path path = scratchPath("unison_replay_file_that_was_never_written.replay");

    const tl::expected<std::vector<std::byte>, unison::Error> read = unison::session::readReplayFile(path);

    REQUIRE_FALSE(read.has_value());
    REQUIRE(read.error().code() == unison::ErrorCode::FileUnavailable);
}
