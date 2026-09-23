#pragma once

#include <unison/runner/scripted_player.hpp>

#include <arena/arena_simulation.hpp>

#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/network_simulator.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/networked_session.hpp>
#include <unison/view/event_dispatcher.hpp>
#include <unison/view/session_runner.hpp>

#include <cstdint>

namespace unison::runner
{

/// One client of a runner match: its own copy of the game, the session it plays through a link over the run's
/// simulated network, the session runner that ticks it from the host's time, and the scripted player at its
/// controls.
class RunnerClient
{
public:
    RunnerClient(net::LoopbackHub& hub,
                 net::NetworkSimulator& network,
                 net::PeerId relay,
                 const net::SessionConfig& config,
                 const net::IClock& clock,
                 std::uint32_t player);

    RunnerClient(const RunnerClient&) = delete;
    RunnerClient& operator=(const RunnerClient&) = delete;
    RunnerClient(RunnerClient&&) = delete;
    RunnerClient& operator=(RunnerClient&&) = delete;

    void join();

    /// Hands the session the scripted player's next input and lets one host frame of time pass.
    void playHostFrame(std::uint64_t hostFrameMicroseconds);

    [[nodiscard]] const session::NetworkedSession& session() const;

    [[nodiscard]] net::PeerId peer() const;

private:
    net::LoopbackEndpoint& endpoint;
    net::SimulatedLink link;
    arena::ArenaSimulation match;
    session::NetworkedSession networked;
    view::EventDispatcher dispatcher;
    view::SessionRunner runner;
    ScriptedPlayer player;
};

}
