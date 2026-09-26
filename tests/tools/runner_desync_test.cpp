#include <unison/runner/runner_match.hpp>

#include <unison/core/error.hpp>
#include <unison/runner/run_outcome.hpp>
#include <unison/runner/runner_options.hpp>
#include <unison/session/file_bytes.hpp>
#include <unison/session/state_diff.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <system_error>
#include <vector>

TEST_CASE("a runner with an injected fault writes dumps that differ in the field it injected")
{
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "unison_runner_desync_dumps";
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    unison::runner::RunnerOptions options;
    options.frames = 30;
    options.faultyClient = 1;
    options.dumpDirectory = directory.string();
    unison::runner::RunnerMatch match{options};
    REQUIRE(match.play().disagreement.has_value());
    const std::vector<tl::expected<std::filesystem::path, unison::Error>> dumps = match.desyncDumps();
    REQUIRE(dumps.size() == 2U);
    REQUIRE(dumps[0].has_value());
    REQUIRE(dumps[1].has_value());
    const tl::expected<std::vector<std::byte>, unison::Error> first = unison::session::readFileBytes(*dumps[0]);
    const tl::expected<std::vector<std::byte>, unison::Error> second = unison::session::readFileBytes(*dumps[1]);
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());

    const tl::expected<std::optional<unison::session::StateDifference>, unison::Error> difference =
        unison::session::firstDifferenceOf(*first, *second);

    REQUIRE(difference.has_value());
    REQUIRE(difference->has_value());
    REQUIRE((*difference)->part == "Health");
    REQUIRE((*difference)->field == "points");
}
