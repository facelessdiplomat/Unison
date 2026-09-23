#include <unison/relay/relay_options.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace
{

tl::expected<unison::relay::RelayOptions, unison::Error> parsed(std::vector<const char*> arguments)
{
    arguments.insert(arguments.begin(), "unison_relay");

    return unison::relay::parseRelayOptions(arguments);
}

}

TEST_CASE("a relay told nothing listens on every interface at port 7777 and runs until it is stopped")
{
    const auto options = parsed({});

    REQUIRE(options.has_value());
    REQUIRE(options->bindAddress == "0.0.0.0");
    REQUIRE(options->port == 7777U);
    REQUIRE(options->maxPeers == 64U);
    REQUIRE(options->inputDeadlineMilliseconds == 100U);
    REQUIRE(options->reliableResendInterval == 10U);
    REQUIRE(options->peerTimeoutMilliseconds == 5000U);
    REQUIRE(options->runForSeconds == 0U);
    REQUIRE_FALSE(options->isHelpAsked);
}

TEST_CASE("a relay takes every option it lists")
{
    const auto options = parsed({"--bind",
                                 "127.0.0.1",
                                 "--port",
                                 "9000",
                                 "--max-peers",
                                 "8",
                                 "--input-deadline",
                                 "150",
                                 "--resend-interval",
                                 "20",
                                 "--peer-timeout",
                                 "2000",
                                 "--run-for",
                                 "3"});

    REQUIRE(options.has_value());
    REQUIRE(options->bindAddress == "127.0.0.1");
    REQUIRE(options->port == 9000U);
    REQUIRE(options->maxPeers == 8U);
    REQUIRE(options->inputDeadlineMilliseconds == 150U);
    REQUIRE(options->reliableResendInterval == 20U);
    REQUIRE(options->peerTimeoutMilliseconds == 2000U);
    REQUIRE(options->runForSeconds == 3U);
}

TEST_CASE("a relay asked for help says so")
{
    const auto options = parsed({"--help"});

    REQUIRE(options.has_value());
    REQUIRE(options->isHelpAsked);
}

TEST_CASE("a relay refuses values it cannot run with, naming the option")
{
    const std::vector<std::pair<std::vector<const char*>, std::string_view>> refusals{
        {{"--max-peers", "0"}, "--max-peers"},
        {{"--input-deadline", "0"}, "--input-deadline"},
        {{"--resend-interval", "0"}, "--resend-interval"},
        {{"--peer-timeout", "0"}, "--peer-timeout"},
    };

    for (const auto& [arguments, option] : refusals)
    {
        CAPTURE(option);

        const auto options = parsed(arguments);

        REQUIRE_FALSE(options.has_value());
        REQUIRE(options.error().code() == unison::ErrorCode::InvalidOption);
        REQUIRE(options.error().message().find(option) != std::string_view::npos);
    }
}

TEST_CASE("the relay's help lists every option")
{
    const std::string help = unison::relay::relayHelp();

    for (const std::string_view option :
         {"--bind", "--port", "--max-peers", "--input-deadline", "--resend-interval", "--peer-timeout", "--run-for"})
    {
        CAPTURE(option);

        REQUIRE(help.find(option) != std::string::npos);
    }
}
