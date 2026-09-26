#include <unison/runner/runner_match.hpp>

#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <unison/sim/asset_hash.hpp>
#include <unison/sim/pipeline_hash.hpp>

#include <algorithm>
#include <limits>
#include <vector>

namespace unison::runner
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;
constexpr std::uint64_t kMicrosecondsPerMillisecond = 1'000;
constexpr std::uint32_t kGraceSeconds = 10;

net::SessionConfig configFor(const RunnerOptions& options)
{
    const arena::ArenaSimulation game{options.players, options.tickRate};
    net::SessionConfig config;
    config.tickRate = options.tickRate;
    config.slotCount = static_cast<std::uint8_t>(options.players);
    config.inputSize = sizeof(arena::ArenaInput);
    config.checksumInterval = options.checksumInterval;
    config.seed = options.seed;
    config.assetHash = sim::hashOf(game.assets());
    config.pipelineHash = sim::hashOf(game.pipeline());

    return config;
}

net::NetworkConditions conditionsFor(const RunnerOptions& options)
{
    return net::NetworkConditions{options.latencyMilliseconds, options.jitterMilliseconds, options.lossRate};
}

std::optional<session::ReplayWriter> recordingFor(const RunnerOptions& options, const net::SessionConfig& config)
{
    if (options.recordPath.empty())
    {
        return std::nullopt;
    }

    return session::ReplayWriter{config};
}

std::deque<RunnerClient> clientsFor(net::LoopbackHub& hub,
                                    net::NetworkSimulator& network,
                                    net::PeerId relay,
                                    const net::SessionConfig& config,
                                    const net::IClock& clock,
                                    session::IVerifiedFrameReceiver* recorder)
{
    std::deque<RunnerClient> clients;

    for (std::uint32_t player = 0; player < config.slotCount; ++player)
    {
        clients.emplace_back(hub, network, relay, config, clock, player, player == 0 ? recorder : nullptr);
    }

    return clients;
}

std::vector<net::PeerId> peersOf(const std::deque<RunnerClient>& clients)
{
    std::vector<net::PeerId> peers;

    for (const RunnerClient& client : clients)
    {
        peers.push_back(client.peer());
    }

    return peers;
}

std::uint32_t lastFrameChecked(const RunnerOptions& options)
{
    return options.frames / options.checksumInterval * options.checksumInterval;
}

}

RunnerMatch::RunnerMatch(const RunnerOptions& options)
    : options{options}, config{configFor(options)}, network{conditionsFor(options), options.seed}, relayEnd{hub.join()},
      relayLink{relayEnd, network}, relay{relayLink, clock, config}, recording{recordingFor(options, config)},
      clients{clientsFor(hub, network, relayEnd.id(), config, clock, recording.has_value() ? &*recording : nullptr)},
      ledger{clients.size()}, wiretap{relay, ledger, peersOf(clients)}
{
}

RunOutcome RunnerMatch::play()
{
    for (RunnerClient& client : clients)
    {
        client.join();
    }

    const std::uint64_t hostFrame = kMicrosecondsPerSecond / options.tickRate;
    const std::uint32_t mostHostFrames = options.frames * 2U + kGraceSeconds * options.tickRate;
    std::uint32_t hostFrames = 0;

    while (hostFrames < mostHostFrames && !hasEveryClientFinished())
    {
        letTimePass(hostFrame);

        relayLink.poll(wiretap);
        relay.update();

        for (RunnerClient& client : clients)
        {
            client.playHostFrame(hostFrame);
        }

        ++hostFrames;
    }

    return outcomeAfter(hostFrames);
}

std::span<const std::byte> RunnerMatch::replay() const
{
    return recording.has_value() ? recording->bytes() : std::span<const std::byte>{};
}

void RunnerMatch::letTimePass(std::uint64_t microseconds)
{
    clock.advance(microseconds);
    elapsedMicroseconds += microseconds;

    const std::uint64_t dueMilliseconds = elapsedMicroseconds / kMicrosecondsPerMillisecond;

    network.advance(static_cast<std::uint32_t>(dueMilliseconds - networkMilliseconds));
    networkMilliseconds = dueMilliseconds;
}

bool RunnerMatch::hasEveryClientFinished() const
{
    const std::uint32_t lastChecked = lastFrameChecked(options);

    return fewestVerifiedFrames() >= options.frames && (lastChecked == 0 || ledger.isReportedByAll(lastChecked));
}

std::uint32_t RunnerMatch::fewestVerifiedFrames() const
{
    std::uint32_t fewest = std::numeric_limits<std::uint32_t>::max();

    for (const RunnerClient& client : clients)
    {
        const session::Session* played = client.session().session();

        fewest = std::min(fewest, played == nullptr ? 0U : played->verifiedFrame());
    }

    return fewest;
}

RunOutcome RunnerMatch::outcomeAfter(std::uint32_t hostFrames) const
{
    RunOutcome outcome;
    outcome.isComplete = hasEveryClientFinished();
    outcome.hostFrames = hostFrames;
    outcome.fewestVerifiedFrames = fewestVerifiedFrames();
    outcome.predictionWindow = config.maxPrediction;
    outcome.framesCompared = ledger.framesReportedByAll();
    outcome.disagreement = ledger.firstDisagreement();

    for (const RunnerClient& client : clients)
    {
        const session::Session* played = client.session().session();

        outcome.clients.push_back(ClientOutcome{
            client.session().localSlot(), played == nullptr ? session::RollbackStats{} : played->rollbackStats()});
    }

    return outcome;
}

}
