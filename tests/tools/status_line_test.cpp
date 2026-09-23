#include <unison/console/status_line.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace
{

unison::console::ConsoleStatus playingStatus()
{
    unison::console::ConsoleStatus status;
    status.name = "ada";
    status.state = unison::session::ConnectionState::Playing;
    status.slot = 1;
    status.verifiedFrame = 312;
    status.predictedFrame = 318;
    status.rollbacksLastSecond = 12;
    status.roundTripMicroseconds = 3'400;
    status.leadMicroseconds = 1'700;

    return status;
}

}

TEST_CASE("the status line of a playing client names its slot, its frames, its rollbacks, its round trip and its lead")
{
    REQUIRE(unison::console::statusLineOf(playingStatus()) ==
            "ada playing in slot 1, verified 312, predicted 318, rollbacks in the last second 12, "
            "round trip 3 ms, lead 1 ms");
}

TEST_CASE("a client behind the relay's clock has a lead below nothing")
{
    unison::console::ConsoleStatus status = playingStatus();
    status.leadMicroseconds = -16'700;

    REQUIRE(unison::console::statusLineOf(status).ends_with("lead -16 ms"));
}

TEST_CASE("the status line of a stalled client says it is stalled")
{
    unison::console::ConsoleStatus status = playingStatus();
    status.state = unison::session::ConnectionState::Stalled;

    REQUIRE(unison::console::statusLineOf(status).find("ada stalled in slot 1") != std::string::npos);
}

TEST_CASE("the status line of a client not in a match yet says where it stands and nothing more")
{
    unison::console::ConsoleStatus status;
    status.name = "ada";

    const std::string idle = unison::console::statusLineOf(status);
    status.state = unison::session::ConnectionState::Connecting;
    const std::string connecting = unison::console::statusLineOf(status);
    status.state = unison::session::ConnectionState::Joining;
    const std::string joining = unison::console::statusLineOf(status);
    status.state = unison::session::ConnectionState::Disconnected;
    const std::string disconnected = unison::console::statusLineOf(status);

    REQUIRE(idle == "ada idle");
    REQUIRE(connecting == "ada connecting to the relay");
    REQUIRE(joining == "ada joining the match");
    REQUIRE(disconnected == "ada disconnected");
}
