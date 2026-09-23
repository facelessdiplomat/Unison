#include <unison/net/loopback_hub.hpp>

#include <unison/core/contract.hpp>

#include <cstdint>
#include <utility>

namespace unison::net
{

LoopbackEndpoint::LoopbackEndpoint(LoopbackHub& hub, PeerId id) : hub{hub}, selfId{id}
{
}

PeerId LoopbackEndpoint::id() const
{
    return selfId;
}

void LoopbackEndpoint::send(PeerId to, Channel channel, std::span<const std::byte> message)
{
    const bool fits = fitsChannel(channel, message.size());

    UNISON_VERIFY(fits);

    if (!fits)
    {
        return;
    }

    hub.carry(selfId, to, channel, message);
}

void LoopbackEndpoint::poll(IMessageReceiver& receiver)
{
    std::swap(inbox, draining);

    for (const Message& message : draining)
    {
        receiver.receive(message.from, message.channel, message.bytes);
    }

    draining.clear();
}

void LoopbackEndpoint::deliver(PeerId from, Channel channel, std::span<const std::byte> message)
{
    inbox.push_back(Message{from, channel, {message.begin(), message.end()}});
}

LoopbackEndpoint& LoopbackHub::join()
{
    return endpoints.emplace_back(*this, PeerId{static_cast<std::uint32_t>(endpoints.size())});
}

void LoopbackHub::carry(PeerId from, PeerId to, Channel channel, std::span<const std::byte> message)
{
    const auto index = static_cast<std::size_t>(to);

    UNISON_VERIFY(index < endpoints.size());

    if (index >= endpoints.size())
    {
        return;
    }

    endpoints[index].deliver(from, channel, message);
}

}
