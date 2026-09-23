#pragma once

#include <unison/runner/runner_client.hpp>
#include <unison/runner/runner_options.hpp>

#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/network_simulator.hpp>
#include <unison/net/relay_core.hpp>
#include <unison/net/session_config.hpp>

#include <cstdint>
#include <deque>

namespace unison::runner
{

/// How a run ended: whether every client verified every frame the run asked for, how many host frames that
/// took, and how far the slowest client got.
struct RunOutcome
{
    bool isComplete = false;
    std::uint32_t hostFrames = 0;
    std::uint32_t fewestVerifiedFrames = 0;
};

/// A match of the runner: every client and the relay in one process, each client over a link through one
/// seeded simulated network, played by scripted players one host frame at a time until every client has
/// verified the frames asked for, or until twice that many host frames and ten seconds more have gone by.
class RunnerMatch
{
public:
    explicit RunnerMatch(const RunnerOptions& options);

    RunnerMatch(const RunnerMatch&) = delete;
    RunnerMatch& operator=(const RunnerMatch&) = delete;
    RunnerMatch(RunnerMatch&&) = delete;
    RunnerMatch& operator=(RunnerMatch&&) = delete;

    [[nodiscard]] RunOutcome play();

private:
    void letTimePass(std::uint64_t microseconds);

    [[nodiscard]] std::uint32_t fewestVerifiedFrames() const;

    RunnerOptions options;
    net::SessionConfig config;
    net::LoopbackHub hub;
    net::NetworkSimulator network;
    net::ManualClock clock;
    net::LoopbackEndpoint& relayEnd;
    net::SimulatedLink relayLink;
    net::RelayCore relay;
    std::deque<RunnerClient> clients;
    std::uint64_t elapsedMicroseconds = 0;
    std::uint64_t networkMilliseconds = 0;
};

}
