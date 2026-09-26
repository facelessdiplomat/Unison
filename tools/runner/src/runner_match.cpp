#include <unison/runner/runner_match.hpp>

#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <unison/sim/asset_hash.hpp>
#include <unison/sim/pipeline_hash.hpp>

#include <algorithm>
#include <limits>
#include <memory>
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

std::vector<std::unique_ptr<RunnerClient>> clientsFor(net::LoopbackHub& hub,
                                                      net::NetworkSimulator& network,
                                                      net::PeerId relay,
                                                      const net::SessionConfig& config,
                                                      const net::IClock& clock,
                                                      const RunnerOptions& options,
                                                      session::IVerifiedFrameReceiver* recorder)
{
    std::vector<std::unique_ptr<RunnerClient>> clients;

    for (std::uint32_t player = 0; player < config.slotCount; ++player)
    {
        const ClientSetup setup{
            player == 0 ? recorder : nullptr, options.dumpDirectory, options.faultyClient == player, 0};
        clients.push_back(std::make_unique<RunnerClient>(hub, network, relay, config, clock, player, setup));
    }

    return clients;
}

std::vector<net::PeerId> peersOf(const std::vector<std::unique_ptr<RunnerClient>>& clients)
{
    std::vector<net::PeerId> peers;

    for (const std::unique_ptr<RunnerClient>& client : clients)
    {
        peers.push_back(client->peer());
    }

    return peers;
}

std::uint32_t lastFrameChecked(const RunnerOptions& options)
{
    return options.frames / options.checksumInterval * options.checksumInterval;
}

std::size_t clientsJoiningAtStart(const RunnerOptions& options)
{
    return options.lateJoinFrame > 0 ? options.players - 1U : options.players;
}

}

RunnerMatch::RunnerMatch(const RunnerOptions& options)
    : options{options}, config{configFor(options)}, network{conditionsFor(options), options.seed}, relayEnd{hub.join()},
      relayLink{relayEnd, network}, relay{relayLink, clock, config}, recording{recordingFor(options, config)},
      clients{clientsFor(
          hub, network, relayEnd.id(), config, clock, options, recording.has_value() ? &*recording : nullptr)},
      ledger{clients.size()}, wiretap{relay, ledger, peersOf(clients)}
{
}

RunOutcome RunnerMatch::play()
{
    for (std::size_t client = 0; client < clientsJoiningAtStart(options); ++client)
    {
        clients[client]->join();
    }

    const std::uint64_t hostFrame = kMicrosecondsPerSecond / options.tickRate;
    const std::uint32_t mostHostFrames = options.frames * 2U + kGraceSeconds * options.tickRate;
    std::uint32_t hostFrames = 0;

    while (hostFrames < mostHostFrames && !hasEveryClientFinished())
    {
        letTimePass(hostFrame);

        relayLink.poll(wiretap);
        relay.update();

        for (const std::unique_ptr<RunnerClient>& client : clients)
        {
            client->playHostFrame(hostFrame);
        }

        joinLateClientWhenDue();
        dropClientWhenDue(hostFrames);
        bringClientBackWhenDue(hostFrames);
        ++hostFrames;
    }

    return outcomeAfter(hostFrames);
}

std::span<const std::byte> RunnerMatch::replay() const
{
    return recording.has_value() ? recording->bytes() : std::span<const std::byte>{};
}

std::vector<tl::expected<std::filesystem::path, Error>> RunnerMatch::desyncDumps() const
{
    std::vector<tl::expected<std::filesystem::path, Error>> dumps;

    for (const std::unique_ptr<RunnerClient>& client : clients)
    {
        if (client->desyncDump().has_value())
        {
            dumps.push_back(*client->desyncDump());
        }
    }

    return dumps;
}

void RunnerMatch::letTimePass(std::uint64_t microseconds)
{
    clock.advance(microseconds);
    elapsedMicroseconds += microseconds;

    const std::uint64_t dueMilliseconds = elapsedMicroseconds / kMicrosecondsPerMillisecond;

    network.advance(static_cast<std::uint32_t>(dueMilliseconds - networkMilliseconds));
    networkMilliseconds = dueMilliseconds;
}

void RunnerMatch::joinLateClientWhenDue()
{
    RunnerClient& lateClient = *clients.back();
    const session::Session* first = clients.front()->session().session();
    const bool isDue = options.lateJoinFrame > 0 && lateClient.session().state() == session::ConnectionState::Idle &&
                       first != nullptr && first->verifiedFrame() >= options.lateJoinFrame;

    if (isDue)
    {
        lateClient.join();
    }
}

void RunnerMatch::dropClientWhenDue(std::uint32_t hostFrame)
{
    const session::Session* first = clients.front()->session().session();
    const bool isDue =
        options.drop.has_value() && !hasDropped && first != nullptr && first->verifiedFrame() >= options.drop->frame;

    if (!isDue)
    {
        return;
    }

    const std::uint32_t client = options.drop->client;
    const ClientSetup setup{nullptr, options.dumpDirectory, false, clients[client]->session().reconnectToken()};
    wiretap.peerLeft(clients[client]->peer());
    clients[client] = std::make_unique<RunnerClient>(hub, network, relayEnd.id(), config, clock, client, setup);
    wiretap.follow(client, clients[client]->peer());
    comesBackAt = hostFrame + options.drop->seconds * options.tickRate;
    hasDropped = true;
}

void RunnerMatch::bringClientBackWhenDue(std::uint32_t hostFrame)
{
    if (!comesBackAt.has_value() || hostFrame < *comesBackAt)
    {
        return;
    }

    clients[options.drop->client]->join();
    comesBackAt.reset();
}

bool RunnerMatch::hasEveryClientFinished() const
{
    const std::uint32_t lastChecked = lastFrameChecked(options);

    return fewestVerifiedFrames() >= options.frames && (lastChecked == 0 || ledger.isReportedByAll(lastChecked));
}

std::uint32_t RunnerMatch::fewestVerifiedFrames() const
{
    std::uint32_t fewest = std::numeric_limits<std::uint32_t>::max();

    for (const std::unique_ptr<RunnerClient>& client : clients)
    {
        const session::Session* played = client->session().session();

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

    for (const std::unique_ptr<RunnerClient>& client : clients)
    {
        const session::Session* played = client->session().session();

        outcome.clients.push_back(
            ClientOutcome{client->session().localSlot(),
                          client->session().startFrame(),
                          client->hasComeBack(),
                          played == nullptr ? session::RollbackStats{} : played->rollbackStats()});
    }

    return outcome;
}

}
