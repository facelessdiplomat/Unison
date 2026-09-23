#pragma once

#include <unison/core/rng.hpp>
#include <unison/net/transport.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unison::net
{

/// What the simulated network does to a message: the one-way delay it adds, how far that delay may stray
/// either way, and the share of unreliable messages it loses.
struct NetworkConditions
{
    std::uint32_t latencyMilliseconds = 0;
    std::uint32_t jitterMilliseconds = 0;
    float lossRate = 0.0F;
};

/// A network that delays, loses and reorders messages as its conditions say, deciding everything from a
/// seed so a run can be played again exactly. Reliable messages are never lost and never overtake one
/// another between the same two ends; time only moves when the network is told to advance.
class NetworkSimulator
{
public:
    NetworkSimulator(const NetworkConditions& conditions, std::uint64_t seed);

    /// Takes a message into the network. Unless it is lost, it leaves through `exit` once it is due.
    void send(ITransport& exit, PeerId to, Channel channel, std::span<const std::byte> message);

    /// Moves the simulated clock on and sends every message due by then through the transport it leaves
    /// by, in the order they fall due.
    void advance(std::uint32_t milliseconds);

private:
    struct InFlight
    {
        std::uint64_t dueAt = 0;
        std::uint64_t sequence = 0;
        ITransport* exit = nullptr;
        PeerId to{};
        Channel channel = Channel::Reliable;
        std::vector<std::byte> bytes;
    };

    struct ReliableLane
    {
        const ITransport* exit = nullptr;
        PeerId to{};
        std::uint64_t lastDueAt = 0;
    };

    [[nodiscard]] std::uint64_t delayFor(Channel channel, const ITransport& exit, PeerId to);

    [[nodiscard]] ReliableLane& laneFor(const ITransport& exit, PeerId to);

    NetworkConditions conditions;
    Rng rng;
    std::uint64_t clock = 0;
    std::uint64_t nextSequence = 0;
    std::vector<InFlight> inFlight;
    std::vector<ReliableLane> reliableLanes;
};

/// A transport whose messages cross the simulated network before the transport it wraps sends them on;
/// what arrives it takes from the wrapped transport unchanged.
class SimulatedLink final : public ITransport
{
public:
    SimulatedLink(ITransport& inner, NetworkSimulator& network);

    void send(PeerId to, Channel channel, std::span<const std::byte> message) override;

    void poll(IMessageReceiver& receiver) override;

private:
    ITransport& inner;
    NetworkSimulator& network;
};

}
