#pragma once

#include <unison/core/error.hpp>
#include <unison/net/transport.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace unison::net
{

/// Where an ENet transport listens: an IPv4 address in dotted form, "0.0.0.0" for every interface, and a
/// port, nought for one the system picks.
struct EnetAddress
{
    std::string host = "127.0.0.1";
    std::uint16_t port = 0;
};

/// A transport over ENet's UDP connections. Our reliable channel is ENet's reliable, ordered one, and our
/// unreliable channel is ENet's unreliable but sequenced one, which drops a message overtaken by a later
/// one. Every peer that connects is named by a peer id no other peer of the transport is ever given; a
/// message sent is on its way at once, and a message for a peer that has gone is dropped, as the network
/// would drop it.
class EnetTransport final : public ITransport
{
    struct Passkey
    {
        explicit Passkey() = default;
    };

    struct Host;

public:
    /// Listens at the address for up to `maxPeers` peers; an address that is none, or that cannot be listened
    /// on, fails, and so does a network the system cannot start.
    [[nodiscard]] static tl::expected<std::unique_ptr<EnetTransport>, Error> listen(const EnetAddress& at,
                                                                                    std::size_t maxPeers);

    EnetTransport(Passkey, std::unique_ptr<Host> host);

    EnetTransport(const EnetTransport&) = delete;
    EnetTransport& operator=(const EnetTransport&) = delete;
    EnetTransport(EnetTransport&&) = delete;
    EnetTransport& operator=(EnetTransport&&) = delete;

    ~EnetTransport() override;

    /// The port the transport listens on, the one the system picked if it was asked for none.
    [[nodiscard]] std::uint16_t port() const;

    void send(PeerId to, Channel channel, std::span<const std::byte> message) override;

    void poll(IMessageReceiver& receiver) override;

private:
    std::unique_ptr<Host> host;
};

}
