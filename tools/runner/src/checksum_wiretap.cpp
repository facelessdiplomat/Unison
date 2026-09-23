#include <unison/runner/checksum_wiretap.hpp>

#include <unison/net/message_codec.hpp>

#include <algorithm>
#include <variant>

namespace unison::runner
{

ChecksumWiretap::ChecksumWiretap(net::IMessageReceiver& relay,
                                 ChecksumLedger& ledger,
                                 std::span<const net::PeerId> clients)
    : relay{relay}, ledger{ledger}, clients{clients.begin(), clients.end()}
{
}

void ChecksumWiretap::receive(net::PeerId from, net::Channel channel, std::span<const std::byte> message)
{
    const auto client = std::ranges::find(clients, from);
    const tl::expected<net::Message, Error> decoded = net::decode(message);

    if (client != clients.end() && decoded.has_value() && std::holds_alternative<net::Checksum>(*decoded))
    {
        const auto& checksum = std::get<net::Checksum>(*decoded);

        ledger.record(static_cast<std::size_t>(client - clients.begin()), checksum.frame, checksum.checksum);
    }

    relay.receive(from, channel, message);
}

void ChecksumWiretap::peerArrived(net::PeerId peer)
{
    relay.peerArrived(peer);
}

void ChecksumWiretap::peerLeft(net::PeerId peer)
{
    relay.peerLeft(peer);
}

}
