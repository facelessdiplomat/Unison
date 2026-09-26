#include <unison/relay/relay_rooms.hpp>

#include <unison/net/message_codec.hpp>

#include <algorithm>
#include <variant>

namespace unison::relay
{

RelayRooms::RelayRooms(net::ITransport& transport,
                       const net::IClock& clock,
                       const net::RelaySettings& settings,
                       const net::IRoundTripMeter* roundTrips)
    : transport{transport}, clock{clock}, settings{settings}, roundTrips{roundTrips}
{
}

void RelayRooms::receive(net::PeerId from, net::Channel channel, std::span<const std::byte> message)
{
    if (const auto seat = seats.find(from); seat != seats.end())
    {
        if (net::RelayCore* room = roomWith(seat->second))
        {
            room->receive(from, channel, message);
        }

        return;
    }

    const tl::expected<net::Message, Error> decoded = net::decode(message);

    if (!decoded.has_value() || !std::holds_alternative<net::Hello>(*decoded))
    {
        return;
    }

    const net::SessionConfig& config = std::get<net::Hello>(*decoded).config;
    net::RelayCore& room = roomFor(config);

    seats.emplace(from, net::hashOf(config));
    room.receive(from, channel, message);
}

void RelayRooms::peerLeft(net::PeerId peer)
{
    const auto seat = seats.find(peer);

    if (seat == seats.end())
    {
        return;
    }

    const std::uint64_t configHash = seat->second;
    seats.erase(seat);

    net::RelayCore* room = roomWith(configHash);

    if (room == nullptr)
    {
        return;
    }

    room->peerLeft(peer);

    if (room->isEmpty())
    {
        std::erase_if(rooms, [configHash](const Room& open) { return open.configHash == configHash; });
    }
}

void RelayRooms::update()
{
    for (const Room& room : rooms)
    {
        room.core->update();
    }
}

std::size_t RelayRooms::roomCount() const
{
    return rooms.size();
}

net::RelayCore& RelayRooms::roomFor(const net::SessionConfig& config)
{
    const std::uint64_t configHash = net::hashOf(config);

    if (net::RelayCore* room = roomWith(configHash))
    {
        return *room;
    }

    return *rooms
                .emplace_back(
                    Room{configHash, std::make_unique<net::RelayCore>(transport, clock, config, settings, roundTrips)})
                .core;
}

net::RelayCore* RelayRooms::roomWith(std::uint64_t configHash)
{
    const auto found = std::ranges::find(rooms, configHash, &Room::configHash);

    return found != rooms.end() ? found->core.get() : nullptr;
}

}
