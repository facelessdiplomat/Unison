#include <unison/runner/runner_match.hpp>

#include <arena/arena_input.hpp>

#include <algorithm>
#include <limits>

namespace unison::runner
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;
constexpr std::uint64_t kMicrosecondsPerMillisecond = 1'000;
constexpr std::uint32_t kGraceSeconds = 10;

net::SessionConfig configFor(const RunnerOptions& options)
{
    net::SessionConfig config;
    config.tickRate = options.tickRate;
    config.slotCount = static_cast<std::uint8_t>(options.players);
    config.inputSize = sizeof(arena::ArenaInput);
    config.checksumInterval = options.checksumInterval;
    config.seed = options.seed;

    return config;
}

net::NetworkConditions conditionsFor(const RunnerOptions& options)
{
    return net::NetworkConditions{options.latencyMilliseconds, options.jitterMilliseconds, options.lossRate};
}

}

RunnerMatch::RunnerMatch(const RunnerOptions& options)
    : options{options}, config{configFor(options)}, network{conditionsFor(options), options.seed}, relayEnd{hub.join()},
      relayLink{relayEnd, network}, relay{relayLink, clock, config}
{
    for (std::uint32_t player = 0; player < options.players; ++player)
    {
        clients.emplace_back(hub, network, relayEnd.id(), config, clock, player);
    }
}

RunOutcome RunnerMatch::play()
{
    for (RunnerClient& client : clients)
    {
        client.join();
    }

    const std::uint64_t hostFrame = kMicrosecondsPerSecond / options.tickRate;
    const std::uint32_t mostHostFrames = options.frames * 2U + kGraceSeconds * options.tickRate;

    RunOutcome outcome;

    while (outcome.hostFrames < mostHostFrames && fewestVerifiedFrames() < options.frames)
    {
        letTimePass(hostFrame);

        relayLink.poll(relay);
        relay.update();

        for (RunnerClient& client : clients)
        {
            client.playHostFrame(hostFrame);
        }

        ++outcome.hostFrames;
    }

    outcome.fewestVerifiedFrames = fewestVerifiedFrames();
    outcome.isComplete = outcome.fewestVerifiedFrames >= options.frames;

    return outcome;
}

void RunnerMatch::letTimePass(std::uint64_t microseconds)
{
    clock.advance(microseconds);
    elapsedMicroseconds += microseconds;

    const std::uint64_t dueMilliseconds = elapsedMicroseconds / kMicrosecondsPerMillisecond;

    network.advance(static_cast<std::uint32_t>(dueMilliseconds - networkMilliseconds));
    networkMilliseconds = dueMilliseconds;
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

}
