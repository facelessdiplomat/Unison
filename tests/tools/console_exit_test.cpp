#include <unison/console/console_exit.hpp>

#include <catch2/catch_test_macros.hpp>

namespace
{

unison::console::ConsoleStatus statusIn(unison::session::ConnectionState state)
{
    unison::console::ConsoleStatus status;
    status.name = "ada";
    status.state = state;

    return status;
}

}

TEST_CASE("a console that played to the end without a desync exits with nought")
{
    REQUIRE(unison::console::exitCodeOf(statusIn(unison::session::ConnectionState::Playing)) == 0);
    REQUIRE(unison::console::exitCodeOf(statusIn(unison::session::ConnectionState::Stalled)) == 0);
}

TEST_CASE("a console that lost the relay exits with one")
{
    REQUIRE(unison::console::exitCodeOf(statusIn(unison::session::ConnectionState::Disconnected)) == 1);
}

TEST_CASE("a console the relay found out of step exits with two, however it left")
{
    unison::console::ConsoleStatus playing = statusIn(unison::session::ConnectionState::Playing);
    playing.desync = unison::net::Desync{1'240, 0b10};
    unison::console::ConsoleStatus disconnected = statusIn(unison::session::ConnectionState::Disconnected);
    disconnected.desync = unison::net::Desync{1'240, 0b10};

    REQUIRE(unison::console::exitCodeOf(playing) == 2);
    REQUIRE(unison::console::exitCodeOf(disconnected) == 2);
}
