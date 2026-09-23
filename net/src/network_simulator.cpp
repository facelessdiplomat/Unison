#include <unison/net/network_simulator.hpp>

#include <algorithm>
#include <utility>

namespace unison::net
{

namespace
{

template <typename Message>
bool fallsDueLater(const Message& left, const Message& right)
{
    return left.dueAt != right.dueAt ? left.dueAt > right.dueAt : left.sequence > right.sequence;
}

}

NetworkSimulator::NetworkSimulator(const NetworkConditions& conditions, std::uint64_t seed)
    : conditions{conditions}, rng{seed}
{
}

void NetworkSimulator::send(ITransport& exit, PeerId to, Channel channel, std::span<const std::byte> message)
{
    if (channel == Channel::Unreliable && rng.nextFloat01() < conditions.lossRate)
    {
        return;
    }

    inFlight.push_back(InFlight{
        clock + delayFor(channel, exit, to), nextSequence++, &exit, to, channel, {message.begin(), message.end()}});
    std::ranges::push_heap(inFlight, fallsDueLater<InFlight>);
}

void NetworkSimulator::advance(std::uint32_t milliseconds)
{
    clock += milliseconds;

    while (!inFlight.empty() && inFlight.front().dueAt <= clock)
    {
        std::ranges::pop_heap(inFlight, fallsDueLater<InFlight>);

        const InFlight due = std::move(inFlight.back());
        inFlight.pop_back();

        due.exit->send(due.to, due.channel, due.bytes);
    }
}

std::uint64_t NetworkSimulator::delayFor(Channel channel, const ITransport& exit, PeerId to)
{
    const auto jitter = static_cast<std::int32_t>(conditions.jitterMilliseconds);
    const std::int64_t delay =
        static_cast<std::int64_t>(conditions.latencyMilliseconds) + rng.nextInRange(-jitter, jitter);
    const auto dueIn = static_cast<std::uint64_t>(std::max<std::int64_t>(delay, 0));

    if (channel == Channel::Unreliable)
    {
        return dueIn;
    }

    ReliableLane& lane = laneFor(exit, to);
    lane.lastDueAt = std::max(lane.lastDueAt, clock + dueIn);

    return lane.lastDueAt - clock;
}

NetworkSimulator::ReliableLane& NetworkSimulator::laneFor(const ITransport& exit, PeerId to)
{
    for (ReliableLane& lane : reliableLanes)
    {
        if (lane.exit == &exit && lane.to == to)
        {
            return lane;
        }
    }

    return reliableLanes.emplace_back(ReliableLane{&exit, to, 0});
}

SimulatedLink::SimulatedLink(ITransport& inner, NetworkSimulator& network) : inner{inner}, network{network}
{
}

void SimulatedLink::send(PeerId to, Channel channel, std::span<const std::byte> message)
{
    network.send(inner, to, channel, message);
}

void SimulatedLink::poll(IMessageReceiver& receiver)
{
    inner.poll(receiver);
}

}
