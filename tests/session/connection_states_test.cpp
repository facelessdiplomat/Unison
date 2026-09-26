#include <unison/session/connection_states.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>

using unison::session::ConnectionState;

TEST_CASE("a client stands idle before anything happens, having moved nowhere")
{
    const unison::session::ConnectionStates states;

    REQUIRE(states.current() == ConnectionState::Idle);
    REQUIRE(states.changes().empty());
}

TEST_CASE("every state a client moves into is noted once, oldest first, and a move to where it stands is not")
{
    unison::session::ConnectionStates states;

    states.moveTo(ConnectionState::Connecting);
    states.moveTo(ConnectionState::Joining);
    states.moveTo(ConnectionState::Joining);
    states.moveTo(ConnectionState::Playing);

    const std::array expected{ConnectionState::Connecting, ConnectionState::Joining, ConnectionState::Playing};
    REQUIRE(states.current() == ConnectionState::Playing);
    REQUIRE(std::ranges::equal(states.changes(), expected));
}

TEST_CASE("clearing the changes forgets them but not where the client stands")
{
    unison::session::ConnectionStates states;
    states.moveTo(ConnectionState::Connecting);

    states.clearChanges();

    REQUIRE(states.changes().empty());
    REQUIRE(states.current() == ConnectionState::Connecting);
}

TEST_CASE("a client is in a match while it plays or stalls, and not while it joins or once it is disconnected")
{
    unison::session::ConnectionStates states;
    states.moveTo(ConnectionState::Joining);
    const bool isInMatchJoining = states.isInMatch();
    states.moveTo(ConnectionState::Playing);
    const bool isInMatchPlaying = states.isInMatch();
    states.moveTo(ConnectionState::Stalled);
    const bool isInMatchStalled = states.isInMatch();
    states.moveTo(ConnectionState::Disconnected);

    REQUIRE_FALSE(isInMatchJoining);
    REQUIRE(isInMatchPlaying);
    REQUIRE(isInMatchStalled);
    REQUIRE_FALSE(states.isInMatch());
}
