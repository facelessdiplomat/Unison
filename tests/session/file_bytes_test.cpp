#include <unison/session/file_bytes.hpp>

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

const std::vector<std::byte> kBytes{
    std::byte{0x55}, std::byte{0x4E}, std::byte{0x52}, std::byte{0x50}, std::byte{0x00}, std::byte{0xFF}};

}

TEST_CASE("a file reads back as the bytes written into it")
{
    const std::filesystem::path path = scratchPath("unison_file_bytes_round_trip.bin");
    const RemovedAfterwards cleanup{path};
    REQUIRE(unison::session::writeFileBytes(path, kBytes).has_value());

    const tl::expected<std::vector<std::byte>, unison::Error> read = unison::session::readFileBytes(path);

    REQUIRE(read.has_value());
    REQUIRE(*read == kBytes);
}

TEST_CASE("a file in a folder that does not exist is not written")
{
    const std::filesystem::path path = scratchPath("unison_file_bytes_no_such_folder") / "match.bin";

    const tl::expected<void, unison::Error> written = unison::session::writeFileBytes(path, kBytes);

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code() == unison::ErrorCode::FileUnavailable);
}

TEST_CASE("a file that does not exist is not read")
{
    const std::filesystem::path path = scratchPath("unison_file_bytes_never_written.bin");

    const tl::expected<std::vector<std::byte>, unison::Error> read = unison::session::readFileBytes(path);

    REQUIRE_FALSE(read.has_value());
    REQUIRE(read.error().code() == unison::ErrorCode::FileUnavailable);
}
