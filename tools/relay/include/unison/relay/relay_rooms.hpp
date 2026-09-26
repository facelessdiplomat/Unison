#pragma once

#include <unison/net/clock.hpp>
#include <unison/net/relay_core.hpp>
#include <unison/net/round_trip_meter.hpp>
#include <unison/net/session_config.hpp>
#include <unison/net/transport.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace unison::relay
{

/// The rooms of a standalone relay, one for every match its clients would play. The first hello of a match
/// opens the room, whose relay core seats the client and answers it from then on, and the room closes once
/// the last of its peers has gone; a peer that has said no hello is not answered.
class RelayRooms final : public net::IMessageReceiver
{
public:
    /// A round-trip meter, when given, tells every room which player to ask for a late joiner's snapshot, and must
    /// outlive the rooms.
    RelayRooms(net::ITransport& transport,
               const net::IClock& clock,
               const net::RelaySettings& settings,
               const net::IRoundTripMeter* roundTrips = nullptr);

    void receive(net::PeerId from, net::Channel channel, std::span<const std::byte> message) override;

    void peerLeft(net::PeerId peer) override;

    /// Lets every room confirm the frames whose deadline has passed.
    void update();

    [[nodiscard]] std::size_t roomCount() const;

private:
    struct Room
    {
        std::uint64_t configHash = 0;
        std::unique_ptr<net::RelayCore> core;
    };

    [[nodiscard]] net::RelayCore& roomFor(const net::SessionConfig& config);

    [[nodiscard]] net::RelayCore* roomWith(std::uint64_t configHash);

    net::ITransport& transport;
    const net::IClock& clock;
    net::RelaySettings settings;
    const net::IRoundTripMeter* roundTrips;
    std::vector<Room> rooms;
    std::unordered_map<net::PeerId, std::uint64_t> seats;
};

}
