#pragma once

#include <unison/net/message_codec.hpp>
#include <unison/net/transport.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace unison::test
{

/// The checksums one client reported, frame and checksum, in the order they reached the relay.
using ChecksumReports = std::vector<std::pair<std::uint32_t, std::uint64_t>>;

/// Hands every message to the relay behind it and keeps, on the way, the checksums two clients reported.
class ChecksumTap final : public net::IMessageReceiver
{
public:
    ChecksumTap(net::IMessageReceiver& relay, net::PeerId first, net::PeerId second)
        : relay{relay}, first{first}, second{second}
    {
    }

    void receive(net::PeerId from, net::Channel channel, std::span<const std::byte> message) override
    {
        const auto decoded = net::decode(message);

        if (decoded.has_value() && std::holds_alternative<net::Checksum>(*decoded))
        {
            const auto& checksum = std::get<net::Checksum>(*decoded);

            (from == first ? firstReports : secondReports).emplace_back(checksum.frame, checksum.checksum);
        }

        relay.receive(from, channel, message);
    }

    void peerLeft(net::PeerId peer) override
    {
        relay.peerLeft(peer);
    }

    ChecksumReports firstReports;
    ChecksumReports secondReports;

private:
    net::IMessageReceiver& relay;
    net::PeerId first;
    net::PeerId second;
};

}
