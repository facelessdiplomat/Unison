#include <unison/net/relay_core.hpp>

#include <unison/core/contract.hpp>
#include <unison/net/message_codec.hpp>

#include <optional>
#include <variant>

namespace unison::net
{

namespace
{

constexpr std::uint32_t kPendingFrames = 128;

}

RelayCore::RelayCore(ITransport& transport,
                     const IClock& clock,
                     const SessionConfig& config,
                     const RelaySettings& settings)
    : clock{clock}, config{config}, settings{settings}, configHash{hashOf(config)}, roster{config.slotCount},
      outbox{transport}, inputs{config.slotCount, config.inputSize, kPendingFrames},
      confirmedLog{std::size_t{config.slotCount} * (1U + config.inputSize)},
      confirmedSlots(std::size_t{config.slotCount} * (1U + config.inputSize))
{
    UNISON_VERIFY(settings.reliableResendInterval > 0);
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

void RelayCore::update()
{
    confirmReadyFrames();
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

    const std::uint8_t slot = roster.freeSlot();

    if (slot == kNoSlot)
    {
        turnAway(from, LeaveReason::RoomFull);

        return;
    }

    admit(from, slot);
}

void RelayCore::handle(PeerId from, const Input& input)
{
    const std::uint8_t slot = roster.slotOf(from);

    if (slot == kNoSlot || input.inputSize != config.inputSize)
    {
        return;
    }

    for (std::uint8_t offset = 0; offset < input.frameCount; ++offset)
    {
        inputs.collect(input.firstFrame + offset,
                       slot,
                       input.inputs.subspan(std::size_t{offset} * input.inputSize, input.inputSize),
                       clock.nowMicroseconds());
    }

    confirmReadyFrames();
}

void RelayCore::handle(PeerId from, const Checksum& checksum)
{
    const std::uint8_t slot = roster.slotOf(from);

    if (slot == kNoSlot)
    {
        return;
    }

    const std::optional<std::uint8_t> minority =
        referee.record(checksum.frame, slot, checksum.checksum, roster.slotsInPlay());

    if (minority.has_value() && *minority != 0U)
    {
        sendToAll(Channel::Reliable, Desync{checksum.frame, *minority});
    }
}

void RelayCore::handle(PeerId from, const Ping& ping)
{
    if (!roster.isMember(from))
    {
        return;
    }

    outbox.send(from, Channel::Unreliable, Pong{ping.sentAt, confirmedLog.lastFrame(), inputs.newestFrame()});
}

void RelayCore::confirmReadyFrames()
{
    const std::uint8_t inPlay = roster.slotsInPlay();

    while (inputs.isNextFrameReady(inPlay) ||
           inputs.isNextFrameOverdue(clock.nowMicroseconds(), settings.inputDeadlineMicroseconds))
    {
        const std::uint32_t frame = inputs.nextFrame();

        inputs.confirmNextFrame(inPlay, confirmedSlots);
        confirmedLog.append(confirmedSlots);
        sendToAll(Channel::Unreliable, Confirmed{frame, config.slotCount, config.inputSize, confirmedSlots});

        if (frame % settings.reliableResendInterval == 0)
        {
            resendReliably(frame);
        }
    }
}

void RelayCore::resendReliably(std::uint32_t lastFrame)
{
    for (std::uint32_t frame = lastFrame - settings.reliableResendInterval + 1; frame <= lastFrame; ++frame)
    {
        sendToAll(Channel::Reliable, Confirmed{frame, config.slotCount, config.inputSize, confirmedLog.slotsOf(frame)});
    }
}

void RelayCore::admit(PeerId peer, std::uint8_t slot)
{
    roster.admit(peer, slot);
    outbox.send(peer, Channel::Reliable, Welcome{slot, config, 0, 0, 0});
}

void RelayCore::turnAway(PeerId peer, LeaveReason reason)
{
    outbox.send(peer, Channel::Reliable, Kick{reason});
}

void RelayCore::sendToAll(Channel channel, const Message& message)
{
    for (const Roster::Member& member : roster.members())
    {
        outbox.send(member.peer, channel, message);
    }
}

}
