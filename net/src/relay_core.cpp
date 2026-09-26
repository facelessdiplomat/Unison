#include <unison/net/relay_core.hpp>

#include <unison/core/contract.hpp>
#include <unison/net/donor_choice.hpp>
#include <unison/net/message_codec.hpp>

#include <algorithm>
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
                     const RelaySettings& settings,
                     const IRoundTripMeter* roundTrips)
    : clock{clock}, config{config}, settings{settings}, roundTrips{roundTrips}, configHash{hashOf(config)},
      roster{config.slotCount}, outbox{transport}, inputs{config.slotCount, config.inputSize, kPendingFrames},
      matchClock{config.tickRate}, confirmedLog{confirmedFrameSize(config.slotCount, config.inputSize)},
      lateJoins{outbox, confirmedLog, config}, reconnectTokens{settings.reconnectTokenSeed},
      confirmedSlots(confirmedFrameSize(config.slotCount, config.inputSize)),
      framesPerDatagram{confirmedFramesPerDatagram(config.slotCount, config.inputSize)}
{
    UNISON_VERIFY(settings.reliableResendInterval > 0);
    UNISON_VERIFY(framesPerDatagram > 0);
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

void RelayCore::peerLeft(PeerId peer)
{
    if (roster.isInPlay(peer))
    {
        roster.holdSlotOf(peer, clock.nowMicroseconds() + settings.reconnectGraceMicroseconds);
    }
    else
    {
        roster.remove(peer);
    }

    lateJoins.forgetJoiner(peer);
    lateJoins.replaceDonor(peer, nearestAwaitedPlayer(roster, roundTrips));
    confirmReadyFrames();
}

void RelayCore::handle(PeerId from, const SnapshotChunk& chunk)
{
    lateJoins.forward(from, chunk);
}

void RelayCore::update()
{
    roster.releaseHeldSlots(clock.nowMicroseconds());
    confirmReadyFrames();
}

bool RelayCore::isEmpty() const
{
    return roster.isEmpty();
}

void RelayCore::handle(PeerId from, const Hello& hello)
{
    if (roster.isMember(from))
    {
        return;
    }

    if (const std::optional<LeaveReason> refusal = refusalOf(hello))
    {
        turnAway(from, *refusal);

        return;
    }

    if (hello.role == Role::Spectator)
    {
        seat(from, kNoSlot);

        return;
    }

    if (roster.slotOfToken(hello.reconnectToken) != kNoSlot)
    {
        readmit(from, hello.reconnectToken);

        return;
    }

    const std::uint8_t slot = roster.freeSlot();

    if (slot == kNoSlot)
    {
        turnAway(from, LeaveReason::RoomFull);

        return;
    }

    seat(from, slot);
}

void RelayCore::handle(PeerId from, const Input& input)
{
    const std::uint8_t slot = roster.slotOf(from);

    if (slot == kNoSlot || input.inputSize != config.inputSize)
    {
        return;
    }

    if (roster.isCatchingUp(from))
    {
        roster.startPlaying(from, std::max(input.firstFrame, inputs.nextFrame()));
    }

    for (std::uint8_t offset = 0; offset < input.frameCount; ++offset)
    {
        inputs.collect(input.firstFrame + offset,
                       slot,
                       input.inputs.subspan(std::size_t{offset} * input.inputSize, input.inputSize),
                       clock.nowMicroseconds());
    }

    if (input.frameCount > 0)
    {
        matchClock.anchor(input.firstFrame + input.frameCount - 1U, clock.nowMicroseconds());
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
        referee.record(checksum.frame, slot, checksum.checksum, roster.slotsAwaitedAt(checksum.frame));

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

    outbox.send(from,
                Channel::Unreliable,
                Pong{ping.sentAt, confirmedLog.lastFrame(), matchClock.dueFrameAt(clock.nowMicroseconds())});
}

void RelayCore::confirmReadyFrames()
{
    while (inputs.isNextFrameReady(roster.slotsAwaitedAt(inputs.nextFrame())) ||
           inputs.isNextFrameOverdue(clock.nowMicroseconds(), settings.inputDeadlineMicroseconds))
    {
        const std::uint32_t frame = inputs.nextFrame();
        const std::uint8_t inPlay = roster.slotsInPlayAt(frame);

        inputs.confirmNextFrame(inPlay, confirmedSlots);
        confirmedLog.append(confirmedSlots);

        const std::uint32_t repeated = std::min({kRedundantConfirmations, framesPerDatagram, frame});
        sendConfirmed(Channel::Unreliable, frame - repeated + 1, frame);

        if (frame % settings.reliableResendInterval == 0)
        {
            resendReliably(frame);
        }
    }
}

void RelayCore::resendReliably(std::uint32_t lastFrame)
{
    for (std::uint32_t first = lastFrame - settings.reliableResendInterval + 1; first <= lastFrame;
         first += framesPerDatagram)
    {
        sendConfirmed(Channel::Reliable, first, std::min(lastFrame, first + framesPerDatagram - 1));
    }
}

void RelayCore::sendConfirmed(Channel channel, std::uint32_t firstFrame, std::uint32_t lastFrame)
{
    sendToAll(channel, confirmationOf(confirmedLog, config, firstFrame, lastFrame - firstFrame + 1));
}

std::optional<LeaveReason> RelayCore::refusalOf(const Hello& hello) const
{
    if (hello.protocolVersion != kProtocolVersion)
    {
        return LeaveReason::ProtocolMismatch;
    }

    if (hashOf(hello.config) != configHash)
    {
        return LeaveReason::ConfigMismatch;
    }

    return std::nullopt;
}

void RelayCore::seat(PeerId peer, std::uint8_t slot)
{
    const std::uint64_t reconnectToken = slot == kNoSlot ? 0 : reconnectTokens.next();
    const std::optional<PeerId> donor = isRunning() ? nearestAwaitedPlayer(roster, roundTrips) : std::nullopt;

    if (!donor.has_value())
    {
        roster.admit(peer, slot, reconnectToken);
        outbox.send(peer, Channel::Reliable, Welcome{slot, config, 0, 0, reconnectToken});

        return;
    }

    roster.admitJoining(peer, slot, reconnectToken);
    lateJoins.await(peer, slot, reconnectToken, *donor);
}

void RelayCore::readmit(PeerId peer, std::uint64_t reconnectToken)
{
    roster.reclaim(reconnectToken, peer);
    const std::uint8_t slot = roster.slotOf(peer);
    const std::optional<PeerId> donor = isRunning() ? nearestAwaitedPlayer(roster, roundTrips) : std::nullopt;

    if (donor.has_value())
    {
        lateJoins.await(peer, slot, reconnectToken, *donor);

        return;
    }

    roster.startPlaying(peer, inputs.nextFrame());
    outbox.send(peer, Channel::Reliable, Welcome{slot, config, 0, 0, reconnectToken});
}

bool RelayCore::isRunning() const
{
    return confirmedLog.lastFrame() > 0;
}

void RelayCore::turnAway(PeerId peer, LeaveReason reason)
{
    outbox.send(peer, Channel::Reliable, Kick{reason});
}

void RelayCore::sendToAll(Channel channel, const Message& message)
{
    for (const Roster::Member& member : roster.members())
    {
        if (!member.heldUntil.has_value())
        {
            outbox.send(member.peer, channel, message);
        }
    }
}

}
