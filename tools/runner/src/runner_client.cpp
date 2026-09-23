#include <unison/runner/runner_client.hpp>

#include <arena/arena_input.hpp>

#include <span>

namespace unison::runner
{

RunnerClient::RunnerClient(net::LoopbackHub& hub,
                           net::NetworkSimulator& network,
                           net::PeerId relay,
                           const net::SessionConfig& config,
                           const net::IClock& clock,
                           std::uint32_t player)
    : endpoint{hub.join()}, link{endpoint, network}, match{config.slotCount, config.tickRate},
      networked{match.frame(), match.pipeline(), config, link, relay},
      runner{networked, dispatcher, clock, config.tickRate}, player{config.seed, player}
{
}

void RunnerClient::join()
{
    networked.join();
}

void RunnerClient::playHostFrame(std::uint64_t hostFrameMicroseconds)
{
    const arena::ArenaInput input = player.nextInput();

    runner.setLocalInput(std::as_bytes(std::span{&input, 1}));
    static_cast<void>(runner.update(hostFrameMicroseconds));
}

const session::NetworkedSession& RunnerClient::session() const
{
    return networked;
}

net::PeerId RunnerClient::peer() const
{
    return endpoint.id();
}

}
