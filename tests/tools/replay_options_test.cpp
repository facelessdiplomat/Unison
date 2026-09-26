#include <unison/replay/replay_options.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace
{

tl::expected<unison::replay::ReplayOptions, unison::Error> parsed(std::vector<const char*> arguments)
{
    arguments.insert(arguments.begin(), "unison_replay");

    return unison::replay::parseReplayOptions(arguments);
}

}

TEST_CASE("a replay command line names the command and the replay file")
{
    const auto verify = parsed({"verify", "match.replay"});
    const auto play = parsed({"play", "match.replay"});

    REQUIRE(verify.has_value());
    REQUIRE(verify->command == unison::replay::ReplayCommand::Verify);
    REQUIRE(verify->file == "match.replay");
    REQUIRE(play.has_value());
    REQUIRE(play->command == unison::replay::ReplayCommand::Play);
    REQUIRE_FALSE(play->isHelpAsked);
}

TEST_CASE("a diff command line names both snapshot files")
{
    const auto options = parsed({"diff", "desync_300_0.snapshot", "desync_300_1.snapshot"});

    REQUIRE(options.has_value());
    REQUIRE(options->command == unison::replay::ReplayCommand::Diff);
    REQUIRE(options->file == "desync_300_0.snapshot");
    REQUIRE(options->otherFile == "desync_300_1.snapshot");
}

TEST_CASE("a diff command line without its second file is refused")
{
    const auto options = parsed({"diff", "desync_300_0.snapshot"});

    REQUIRE_FALSE(options.has_value());
    REQUIRE(options.error().code() == unison::ErrorCode::InvalidOption);
}

TEST_CASE("a replay command line without a file is refused")
{
    const auto options = parsed({"verify"});

    REQUIRE_FALSE(options.has_value());
    REQUIRE(options.error().code() == unison::ErrorCode::InvalidOption);
}

TEST_CASE("a replay command the tool does not know is refused")
{
    const auto options = parsed({"rewind", "match.replay"});

    REQUIRE_FALSE(options.has_value());
    REQUIRE(options.error().code() == unison::ErrorCode::InvalidOption);
}

TEST_CASE("the replay tool asked for help says so")
{
    const auto options = parsed({"--help"});

    REQUIRE(options.has_value());
    REQUIRE(options->isHelpAsked);
}

TEST_CASE("the replay tool's help names every command")
{
    const std::string help = unison::replay::replayHelp();

    for (const std::string_view command : {"play", "verify", "diff"})
    {
        CAPTURE(command);

        REQUIRE(help.find(command) != std::string::npos);
    }
}
