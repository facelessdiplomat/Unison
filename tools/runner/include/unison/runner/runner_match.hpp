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
#include <unison/session/replay_writer.hpp>

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace unison::runner
{

/// A match of the runner: every client and the relay in one process, each client over a link through one
/// seeded simulated network, played by scripted players one host frame at a time. It ends once every client
/// has verified the frames asked for and reported the checksums of them, or fails after twice as many host
/// frames and ten seconds more; on its way to the relay every checksum is written into a ledger. A run with a
/// file to record into records its first client's replay, one with a frame to join late at lets its last client
/// join once the first has verified that frame, and one with a drop takes a client off the network once the first has
/// verified the frame of the drop, in its place a new client over a new link that records no replay and joins with the
/// old one's reconnect token the seconds of the drop later.
class RunnerMatch
{
public:
    explicit RunnerMatch(const RunnerOptions& options);

    RunnerMatch(const RunnerMatch&) = delete;
    RunnerMatch& operator=(const RunnerMatch&) = delete;
    RunnerMatch(RunnerMatch&&) = delete;
    RunnerMatch& operator=(RunnerMatch&&) = delete;

    [[nodiscard]] RunOutcome play();

    /// The replay recorded so far, no bytes for a run without a file to record into.
    [[nodiscard]] std::span<const std::byte> replay() const;

    /// The file every client that heard of a desync dumped its snapshot into, or why it could not, in client order.
    [[nodiscard]] std::vector<tl::expected<std::filesystem::path, Error>> desyncDumps() const;

private:
    void letTimePass(std::uint64_t microseconds);

    void joinLateClientWhenDue();

    void dropClientWhenDue(std::uint32_t hostFrame);

    void bringClientBackWhenDue(std::uint32_t hostFrame);

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
    std::optional<session::ReplayWriter> recording;
    std::vector<std::unique_ptr<RunnerClient>> clients;
    ChecksumLedger ledger;
    ChecksumWiretap wiretap;
    std::uint64_t elapsedMicroseconds = 0;
    std::uint64_t networkMilliseconds = 0;
    bool hasDropped = false;
    std::optional<std::uint32_t> comesBackAt;
};

}
