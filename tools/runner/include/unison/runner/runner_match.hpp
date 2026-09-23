#pragma once

#include <unison/runner/checksum_ledger.hpp>
#include <unison/runner/checksum_wiretap.hpp>
#include <unison/runner/run_outcome.hpp>
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

/// A match of the runner: every client and the relay in one process, each client over a link through one
/// seeded simulated network, played by scripted players one host frame at a time. It ends once every client
/// has verified the frames asked for and reported the checksums of them, or fails after twice as many host
/// frames and ten seconds more; on its way to the relay every checksum is written into a ledger.
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

    [[nodiscard]] bool hasEveryClientFinished() const;

    [[nodiscard]] std::uint32_t fewestVerifiedFrames() const;

    [[nodiscard]] RunOutcome outcomeAfter(std::uint32_t hostFrames) const;

    RunnerOptions options;
    net::SessionConfig config;
    net::LoopbackHub hub;
    net::NetworkSimulator network;
    net::ManualClock clock;
    net::LoopbackEndpoint& relayEnd;
    net::SimulatedLink relayLink;
    net::RelayCore relay;
    std::deque<RunnerClient> clients;
    ChecksumLedger ledger;
    ChecksumWiretap wiretap;
    std::uint64_t elapsedMicroseconds = 0;
    std::uint64_t networkMilliseconds = 0;
};

}
