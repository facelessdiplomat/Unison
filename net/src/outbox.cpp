#include <unison/net/outbox.hpp>

#include <unison/core/contract.hpp>
#include <unison/net/message_codec.hpp>

#include <span>

namespace unison::net
{

Outbox::Outbox(ITransport& transport) : transport{transport}
{
}

void Outbox::send(PeerId to, Channel channel, const Message& message)
{
    const tl::expected<std::size_t, Error> written = encode(message, buffer);

    UNISON_VERIFY(written.has_value());

    if (!written.has_value())
    {
        return;
    }

    transport.send(to, channel, std::span{buffer}.first(*written));
}

}
