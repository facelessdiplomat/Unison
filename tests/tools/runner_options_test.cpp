#include <unison/runner/runner_options.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace
{

tl::expected<unison::runner::RunnerOptions, unison::Error> parsed(std::vector<const char*> arguments)
{
    arguments.insert(arguments.begin(), "unison_runner");

    return unison::runner::parseRunnerOptions(arguments);
}

struct Refusal
{
    std::vector<const char*> arguments;
    std::string_view option;
};

}

TEST_CASE("a runner told nothing plays two players for six hundred frames on a flawless network")
{
    const auto options = parsed({});

    REQUIRE(options.has_value());
    REQUIRE(options->players == 2U);
    REQUIRE(options->frames == 600U);
    REQUIRE(options->latencyMilliseconds == 0U);
    REQUIRE(options->jitterMilliseconds == 0U);
    REQUIRE(options->lossRate == 0.0F);
    REQUIRE(options->tickRate == 60U);
    REQUIRE(options->checksumInterval == 1U);
    REQUIRE(options->recordPath.empty());
    REQUIRE(options->dumpDirectory == ".");
    REQUIRE_FALSE(options->faultyClient.has_value());
    REQUIRE_FALSE(options->isHelpAsked);
}

TEST_CASE("every option of the runner is read")
{
    const auto options = parsed({"--players", "4",         "--frames",     "36000",      "--seed",
                                 "77",        "--latency", "120",          "--jitter",   "30",
                                 "--loss",    "5",         "--tick-rate",  "30",         "--checksum-interval",
                                 "20",        "--record",  "match.replay", "--dump-dir", "dumps",
                                 "--fault",   "3"});

    REQUIRE(options.has_value());
    REQUIRE(options->players == 4U);
    REQUIRE(options->frames == 36000U);
    REQUIRE(options->seed == 77U);
    REQUIRE(options->latencyMilliseconds == 120U);
    REQUIRE(options->jitterMilliseconds == 30U);
    REQUIRE_THAT(options->lossRate, Catch::Matchers::WithinAbs(0.05, 0.0001));
    REQUIRE(options->tickRate == 30U);
    REQUIRE(options->checksumInterval == 20U);
    REQUIRE(options->recordPath == "match.replay");
    REQUIRE(options->dumpDirectory == "dumps");
    REQUIRE(options->faultyClient == 3U);
}

TEST_CASE("a runner asked for help says so and lists its options")
{
    const auto options = parsed({"--help"});

    REQUIRE(options.has_value());
    REQUIRE(options->isHelpAsked);
    REQUIRE(unison::runner::runnerHelp().find("--checksum-interval") != std::string::npos);
}

TEST_CASE("a run the runner cannot play is refused with an error naming the option")
{
    const std::array refusals{Refusal{{"--players", "0"}, "--players"},
                              Refusal{{"--players", "9"}, "--players"},
                              Refusal{{"--frames", "0"}, "--frames"},
                              Refusal{{"--loss", "101"}, "--loss"},
                              Refusal{{"--loss", "-1"}, "--loss"},
                              Refusal{{"--tick-rate", "0"}, "--tick-rate"},
                              Refusal{{"--checksum-interval", "0"}, "--checksum-interval"},
                              Refusal{{"--fault", "2"}, "--fault"}};

    for (const Refusal& refusal : refusals)
    {
        const auto options = parsed(refusal.arguments);

        REQUIRE_FALSE(options.has_value());
        REQUIRE(options.error().code() == unison::ErrorCode::InvalidOption);
        REQUIRE(options.error().message().find(refusal.option) != std::string_view::npos);
    }
}
