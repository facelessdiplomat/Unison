#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace unison::net
{

/// Names one end of a connection, as the transport that carries it sees it.
enum class PeerId : std::uint32_t
{
};

/// How a message travels. A reliable message arrives once and in order; an unreliable one may be lost,
/// late or overtaken, and carries what is sent again anyway.
enum class Channel : std::uint8_t
{
    Reliable,
    Unreliable
};

/// Takes the messages a transport hands over when it is polled.
class IMessageReceiver
{
public:
    IMessageReceiver() = default;
    virtual ~IMessageReceiver() = default;

    IMessageReceiver(const IMessageReceiver&) = delete;
    IMessageReceiver& operator=(const IMessageReceiver&) = delete;
    IMessageReceiver(IMessageReceiver&&) = delete;
    IMessageReceiver& operator=(IMessageReceiver&&) = delete;

    virtual void receive(PeerId from, Channel channel, std::span<const std::byte> message) = 0;
};

/// Carries messages between peers. What a message says is the protocol's business, never the transport's.
class ITransport
{
public:
    ITransport() = default;
    virtual ~ITransport() = default;

    ITransport(const ITransport&) = delete;
    ITransport& operator=(const ITransport&) = delete;
    ITransport(ITransport&&) = delete;
    ITransport& operator=(ITransport&&) = delete;

    virtual void send(PeerId to, Channel channel, std::span<const std::byte> message) = 0;

    /// Hands every message that arrived since the last poll to the receiver, oldest first.
    virtual void poll(IMessageReceiver& receiver) = 0;
};

}
