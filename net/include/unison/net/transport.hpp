#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace unison::net
{

/// The most bytes an unreliable message may carry: small enough to cross the internet in one datagram, so
/// no transport has to split it and make its loss depend on every piece arriving.
inline constexpr std::size_t kMaxUnreliableMessageSize = 1200;

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

/// Whether a message of `size` bytes may go on the channel: any may go reliably, and none longer than
/// `kMaxUnreliableMessageSize` unreliably.
[[nodiscard]] constexpr bool fitsChannel(Channel channel, std::size_t size)
{
    return channel == Channel::Reliable || size <= kMaxUnreliableMessageSize;
}

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

    /// A peer's connection is up: a client's to its server, or a server's to a client. A transport that has
    /// no connections, as an in-process one, never says so. A receiver that does not follow peers leaves it
    /// alone; one that hands messages on hands this on too.
    virtual void peerArrived(PeerId peer)
    {
        static_cast<void>(peer);
    }

    /// A peer has gone: it said goodbye, or it stopped answering for longer than the transport waits. A
    /// receiver that does not follow peers leaves it alone; one that hands messages on hands this on too.
    virtual void peerLeft(PeerId peer)
    {
        static_cast<void>(peer);
    }
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

    /// Sends a message of any length on the reliable channel; one longer than `kMaxUnreliableMessageSize` on
    /// the unreliable channel breaks a contract and is not sent.
    virtual void send(PeerId to, Channel channel, std::span<const std::byte> message) = 0;

    /// Hands every message that arrived since the last poll to the receiver, oldest first.
    virtual void poll(IMessageReceiver& receiver) = 0;
};

}
