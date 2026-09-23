#pragma once

#include <unison/net/transport.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace unison::test
{

/// A transport that can be cut off. While it is down, unreliable messages are lost both ways and reliable ones
/// wait, as an outage treats them on a real network; once it is up again the waiting ones go through first,
/// in the order they were sent.
class OutageLink final : public net::ITransport
{
public:
    explicit OutageLink(net::ITransport& inner) : inner{inner}
    {
    }

    void cut()
    {
        isDown = true;
    }

    void restore()
    {
        isDown = false;

        for (const Held& message : heldOutgoing)
        {
            inner.send(message.peer, net::Channel::Reliable, message.bytes);
        }

        heldOutgoing.clear();
    }

    void send(net::PeerId to, net::Channel channel, std::span<const std::byte> message) override
    {
        if (!isDown)
        {
            inner.send(to, channel, message);
        }
        else if (channel == net::Channel::Reliable)
        {
            heldOutgoing.push_back(Held{to, {message.begin(), message.end()}});
        }
    }

    void poll(net::IMessageReceiver& receiver) override
    {
        if (isDown)
        {
            inner.poll(incoming);

            return;
        }

        for (const Held& message : incoming.held)
        {
            receiver.receive(message.peer, net::Channel::Reliable, message.bytes);
        }

        incoming.held.clear();
        inner.poll(receiver);
    }

private:
    struct Held
    {
        net::PeerId peer{};
        std::vector<std::byte> bytes;
    };

    class ReliableKeeper final : public net::IMessageReceiver
    {
    public:
        void receive(net::PeerId from, net::Channel channel, std::span<const std::byte> message) override
        {
            if (channel == net::Channel::Reliable)
            {
                held.push_back(Held{from, {message.begin(), message.end()}});
            }
        }

        std::vector<Held> held;
    };

    net::ITransport& inner;
    ReliableKeeper incoming;
    std::vector<Held> heldOutgoing;
    bool isDown = false;
};

}
