#include <unison/console/console_options.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace
{

tl::expected<unison::console::ConsoleOptions, unison::Error> parsed(std::vector<const char*> arguments)
{
    arguments.insert(arguments.begin(), "unison_console");

    return unison::console::parseConsoleOptions(arguments);
}

}

TEST_CASE("a console told nothing joins a relay on this machine at port 7777 and runs until it is stopped")
{
    const auto options = parsed({});

    REQUIRE(options.has_value());
    REQUIRE(options->host == "127.0.0.1");
    REQUIRE(options->port == 7777U);
    REQUIRE(options->from.empty());
    REQUIRE(options->name == "player");
    REQUIRE(options->players == 2U);
    REQUIRE(options->runForSeconds == 0U);
    REQUIRE(options->recordPath.empty());
    REQUIRE(options->dumpDirectory == ".");
    REQUIRE_FALSE(options->isSpectating);
    REQUIRE(options->spectatorDelayFrames == 0U);
    REQUIRE_FALSE(options->isHelpAsked);
}

TEST_CASE("a console takes every option it lists")
{
    const auto options = parsed({"--host",
                                 "192.168.1.20",
                                 "--port",
                                 "9000",
                                 "--from",
                                 "192.168.1.30",
                                 "--name",
                                 "ada",
                                 "--players",
                                 "4",
                                 "--run-for",
                                 "5",
                                 "--record",
                                 "match.replay",
                                 "--dump-dir",
                                 "dumps",
                                 "--spectate",
                                 "--delay",
                                 "10"});

    REQUIRE(options.has_value());
    REQUIRE(options->host == "192.168.1.20");
    REQUIRE(options->port == 9000U);
    REQUIRE(options->from == "192.168.1.30");
    REQUIRE(options->name == "ada");
    REQUIRE(options->players == 4U);
    REQUIRE(options->runForSeconds == 5U);
    REQUIRE(options->recordPath == "match.replay");
    REQUIRE(options->dumpDirectory == "dumps");
    REQUIRE(options->isSpectating);
    REQUIRE(options->spectatorDelayFrames == 10U);
}

TEST_CASE("a console asked for help says so")
{
    const auto options = parsed({"--help"});

    REQUIRE(options.has_value());
    REQUIRE(options->isHelpAsked);
}

TEST_CASE("a console refuses a match it cannot play, naming the option")
{
    for (const char* players : {"0", "9"})
    {
        CAPTURE(players);

        const auto options = parsed({"--players", players});

        REQUIRE_FALSE(options.has_value());
        REQUIRE(options.error().code() == unison::ErrorCode::InvalidOption);
        REQUIRE(options.error().message().find("--players") != std::string_view::npos);
    }
}

TEST_CASE("a console refuses a delay unless it spectates, naming the option")
{
    const auto options = parsed({"--delay", "10"});

    REQUIRE_FALSE(options.has_value());
    REQUIRE(options.error().code() == unison::ErrorCode::InvalidOption);
    REQUIRE(options.error().message().find("--delay") != std::string_view::npos);
}

TEST_CASE("the console's help lists every option")
{
    const std::string help = unison::console::consoleHelp();

    for (const std::string_view option : {"--host",
                                          "--port",
                                          "--from",
                                          "--name",
                                          "--players",
                                          "--run-for",
                                          "--record",
                                          "--dump-dir",
                                          "--spectate",
                                          "--delay"})
    {
        CAPTURE(option);

        REQUIRE(help.find(option) != std::string::npos);
    }
}
