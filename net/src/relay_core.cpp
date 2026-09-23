#include <unison/net/relay_core.hpp>

#include <unison/core/contract.hpp>
#include <unison/net/message_codec.hpp>

#include <algorithm>
#include <variant>

namespace unison::net
{

RelayCore::RelayCore(ITransport& transport, const SessionConfig& config)
    : transport{transport}, config{config}, configHash{hashOf(config)}
{
}

void RelayCore::receive(PeerId from, Channel, std::span<const std::byte> message)
{
    const tl::expected<Message, Error> decoded = decode(message);

    if (!decoded.has_value())
    {
        return;
    }

    std::visit([this, from](const auto& received) { handle(from, received); }, *decoded);
}

void RelayCore::handle(PeerId from, const Hello& hello)
{
    if (hello.protocolVersion != kProtocolVersion)
    {
        turnAway(from, LeaveReason::ProtocolMismatch);

        return;
    }

    if (hello.configHash != configHash)
    {
        turnAway(from, LeaveReason::ConfigMismatch);

        return;
    }

    if (hello.role == Role::Spectator)
    {
        admit(from, kNoSlot);

        return;
    }

    const std::uint8_t slot = freeSlot();

    if (slot == kNoSlot)
    {
        turnAway(from, LeaveReason::RoomFull);

        return;
    }

    admit(from, slot);
}

void RelayCore::admit(PeerId peer, std::uint8_t slot)
{
    members.push_back(Member{peer, slot});
    sendTo(peer, Channel::Reliable, Welcome{slot, config, 0, 0, 0});
}

void RelayCore::turnAway(PeerId peer, LeaveReason reason)
{
    sendTo(peer, Channel::Reliable, Kick{reason});
}

void RelayCore::sendTo(PeerId peer, Channel channel, const Message& message)
{
    const tl::expected<std::size_t, Error> written = encode(message, sendBuffer);

    UNISON_VERIFY(written.has_value());

    if (!written.has_value())
    {
        return;
    }

    transport.send(peer, channel, std::span{sendBuffer}.first(*written));
}

std::uint8_t RelayCore::freeSlot() const
{
    for (std::uint8_t slot = 0; slot < config.slotCount; ++slot)
    {
        const bool isTaken = std::ranges::any_of(members, [slot](const Member& member) { return member.slot == slot; });

        if (!isTaken)
        {
            return slot;
        }
    }

    return kNoSlot;
}

}
