#pragma once

#include <unison/net/transport.hpp>
#include <unison/runner/checksum_ledger.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace unison::runner
{

/// Stands in front of the relay: it hands every message on and, on the way, writes the checksums the clients
/// report into a ledger under the client that sent them, the clients numbered in the order of their peers.
/// Messages from anyone else, and those it cannot read, go on untouched.
class ChecksumWiretap final : public net::IMessageReceiver
{
public:
    ChecksumWiretap(net::IMessageReceiver& relay, ChecksumLedger& ledger, std::span<const net::PeerId> clients);

    void receive(net::PeerId from, net::Channel channel, std::span<const std::byte> message) override;

private:
    net::IMessageReceiver& relay;
    ChecksumLedger& ledger;
    std::vector<net::PeerId> clients;
};

}
