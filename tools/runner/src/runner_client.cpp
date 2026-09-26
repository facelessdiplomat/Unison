#include <unison/runner/runner_client.hpp>

#include <arena/arena_input.hpp>
#include <arena/components.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace unison::runner
{

namespace
{

constexpr std::uint32_t kNoInputDelay = 0;
constexpr std::uint8_t kFaultedPlayer = 0;

std::optional<session::DesyncDumper> dumperFor(const ClientSetup& setup)
{
    if (setup.dumpDirectory.empty())
    {
        return std::nullopt;
    }

    return session::DesyncDumper{setup.dumpDirectory};
}

std::vector<session::IVerifiedFrameReceiver*> receiversOf(const ClientSetup& setup,
                                                          std::optional<session::DesyncDumper>& dumper)
{
    std::vector<session::IVerifiedFrameReceiver*> receivers;

    if (setup.recorder != nullptr)
    {
        receivers.push_back(setup.recorder);
    }

    if (dumper.has_value())
    {
        receivers.push_back(&*dumper);
    }

    return receivers;
}

void lowerTheFirstPlayersHealth(sim::Frame& frame)
{
    for (auto [entity, slot, health] : frame.registry.view<arena::PlayerSlot, arena::Health>().each())
    {
        if (slot.slot == kFaultedPlayer)
        {
            --health.points;
        }
    }
}

}

RunnerClient::RunnerClient(net::LoopbackHub& hub,
                           net::NetworkSimulator& network,
                           net::PeerId relay,
                           const net::SessionConfig& config,
                           const net::IClock& clock,
                           std::uint32_t player,
                           const ClientSetup& setup)
    : endpoint{hub.join()}, link{endpoint, network}, match{config.slotCount, config.tickRate}, dumper{dumperFor(setup)},
      verifiedFrames{receiversOf(setup, dumper)},
      networked{match.frame(), match.pipeline(), config, link, relay, kNoInputDelay, &verifiedFrames},
      runner{networked, dispatcher, clock, config.tickRate}, player{config.seed, player},
      reconnectToken{setup.reconnectToken}, isSpectator{setup.isSpectator}
{
    if (setup.isFaulty)
    {
        lowerTheFirstPlayersHealth(match.frame());
    }
}

void RunnerClient::join()
{
    if (isSpectator)
    {
        networked.spectate();

        return;
    }

    networked.join(reconnectToken);
}

bool RunnerClient::hasComeBack() const
{
    return reconnectToken != 0;
}

void RunnerClient::playHostFrame(std::uint64_t hostFrameMicroseconds)
{
    const arena::ArenaInput input = player.nextInput();

    runner.setLocalInput(std::as_bytes(std::span{&input, 1}));
    static_cast<void>(runner.update(hostFrameMicroseconds));

    const std::optional<net::Desync> desync = networked.lastDesync();

    if (dumper.has_value() && desync.has_value() && !dumped.has_value())
    {
        dumped = dumper->dump(desync->frame, networked.localSlot());
    }
}

const std::optional<tl::expected<std::filesystem::path, Error>>& RunnerClient::desyncDump() const
{
    return dumped;
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
