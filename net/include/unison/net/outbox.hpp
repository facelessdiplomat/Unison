#pragma once

#include <unison/net/protocol.hpp>
#include <unison/net/transport.hpp>

#include <array>
#include <cstddef>

namespace unison::net
{

/// Writes messages the way the wire carries them and hands them to a transport. Every message the protocol
/// sends fits in a datagram, so one that does not breaks a contract and is not sent.
class Outbox
{
public:
    explicit Outbox(ITransport& transport);

    void send(PeerId to, Channel channel, const Message& message);

private:
    ITransport& transport;
    std::array<std::byte, kMaxDatagramSize> buffer{};
};

}
