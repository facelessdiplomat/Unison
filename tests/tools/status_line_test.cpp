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

TEST_CASE("the status line of a client the relay found out of step names the frame and the slot")
{
    unison::console::ConsoleStatus status = playingStatus();
    status.desync = unison::net::Desync{1'240, 0b10};

    REQUIRE(unison::console::statusLineOf(status).ends_with("lead 1 ms, desync on frame 1240 by slot 1"));
}

TEST_CASE("a desync of several slots names every one of them")
{
    unison::console::ConsoleStatus status = playingStatus();
    status.desync = unison::net::Desync{1'240, 0b101};

    REQUIRE(unison::console::statusLineOf(status).ends_with("desync on frame 1240 by slots 0, 2"));
}

TEST_CASE("a client that left its match still tells of the desync it was told of")
{
    unison::console::ConsoleStatus status = playingStatus();
    status.state = unison::session::ConnectionState::Disconnected;
    status.desync = unison::net::Desync{1'240, 0b10};

    REQUIRE(unison::console::statusLineOf(status) == "ada disconnected, desync on frame 1240 by slot 1");
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

TEST_CASE("the status line of a spectator says it watches, stalled or not, and names no slot")
{
    unison::console::ConsoleStatus status = playingStatus();
    status.slot = unison::net::kNoSlot;
    status.predictedFrame = status.verifiedFrame;
    unison::console::ConsoleStatus waiting = status;
    waiting.state = unison::session::ConnectionState::Stalled;

    REQUIRE(unison::console::statusLineOf(status) ==
            "ada watching, verified 312, predicted 312, rollbacks in the last second 12, round trip 3 ms, lead 1 ms");
    REQUIRE(unison::console::statusLineOf(waiting).starts_with("ada watching, verified 312"));
}
