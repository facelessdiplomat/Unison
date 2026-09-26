#pragma once

#include <unison/runner/scripted_player.hpp>

#include <arena/arena_simulation.hpp>

#include <unison/core/error.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/network_simulator.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/desync_dumper.hpp>
#include <unison/session/networked_session.hpp>
#include <unison/session/verified_frame_fan_out.hpp>
#include <unison/session/verified_frame_receiver.hpp>
#include <unison/view/event_dispatcher.hpp>
#include <unison/view/session_runner.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>

namespace unison::runner
{

/// What a client of a runner match does besides playing: records every frame it verifies into a receiver, when given,
/// dumps the snapshot of a desync into a folder, when given, starts with the first player one health point low when it
/// is the faulty one, joins with the reconnect token of an earlier welcome when it comes back after a drop, and only
/// watches when it is a spectator.
struct ClientSetup
{
    session::IVerifiedFrameReceiver* recorder = nullptr;
    std::filesystem::path dumpDirectory;
    bool isFaulty = false;
    std::uint64_t reconnectToken = 0;
    bool isSpectator = false;
};

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
                 std::uint32_t player,
                 const ClientSetup& setup = {});

    RunnerClient(const RunnerClient&) = delete;
    RunnerClient& operator=(const RunnerClient&) = delete;
    RunnerClient(RunnerClient&&) = delete;
    RunnerClient& operator=(RunnerClient&&) = delete;

    /// Asks the relay to let the client in, back into the slot its reconnect token was handed with when it has one, or
    /// to let it watch when it is a spectator.
    void join();

    /// Whether the client comes back after a drop, with a reconnect token.
    [[nodiscard]] bool hasComeBack() const;

    /// Hands the session the scripted player's next input, lets one host frame of time pass, and dumps the snapshot of
    /// the first desync the relay reports.
    void playHostFrame(std::uint64_t hostFrameMicroseconds);

    /// The file the snapshot of the first desync went into, or why it did not; nothing before a desync or without a
    /// folder to dump into.
    [[nodiscard]] const std::optional<tl::expected<std::filesystem::path, Error>>& desyncDump() const;

    [[nodiscard]] const session::NetworkedSession& session() const;

    [[nodiscard]] net::PeerId peer() const;

private:
    net::LoopbackEndpoint& endpoint;
    net::SimulatedLink link;
    arena::ArenaSimulation match;
    std::optional<session::DesyncDumper> dumper;
    session::VerifiedFrameFanOut verifiedFrames;
    session::NetworkedSession networked;
    view::EventDispatcher dispatcher;
    view::SessionRunner runner;
    ScriptedPlayer player;
    std::uint64_t reconnectToken;
    bool isSpectator;
    std::optional<tl::expected<std::filesystem::path, Error>> dumped;
};

}
