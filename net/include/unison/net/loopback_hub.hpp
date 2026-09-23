#pragma once

#include <unison/net/transport.hpp>

#include <cstddef>
#include <deque>
#include <span>
#include <vector>

namespace unison::net
{

class LoopbackHub;

/// One peer of a loopback hub. What it sends lands in the inbox of the peer it names, and a poll empties
/// its own inbox; a message sent while it is being polled waits for the next poll.
class LoopbackEndpoint final : public ITransport
{
public:
    LoopbackEndpoint(LoopbackHub& hub, PeerId id);

    [[nodiscard]] PeerId id() const;

    void send(PeerId to, Channel channel, std::span<const std::byte> message) override;

    void poll(IMessageReceiver& receiver) override;

    /// Leaves a message in the inbox; the hub calls it for every message it carries here.
    void deliver(PeerId from, Channel channel, std::span<const std::byte> message);

private:
    struct Message
    {
        PeerId from{};
        Channel channel = Channel::Reliable;
        std::vector<std::byte> bytes;
    };

    LoopbackHub& hub;
    PeerId selfId;
    std::vector<Message> inbox;
    std::vector<Message> draining;
};

/// An in-process network whose endpoints hand messages to one another at once, whole and in order, on
/// both channels. Endpoints are numbered in the order they join and stay where they are.
class LoopbackHub
{
public:
    [[nodiscard]] LoopbackEndpoint& join();

    /// Carries a message to the endpoint it names; naming an endpoint the hub does not have breaks a
    /// contract.
    void carry(PeerId from, PeerId to, Channel channel, std::span<const std::byte> message);

private:
    std::deque<LoopbackEndpoint> endpoints;
};

}
