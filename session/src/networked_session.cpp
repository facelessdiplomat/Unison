#include <unison/session/networked_session.hpp>

#include <unison/core/contract.hpp>
#include <unison/net/message_codec.hpp>
#include <unison/session/confirmed_inputs.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <algorithm>
#include <variant>
#include <vector>

namespace unison::session
{

namespace
{

std::vector<IVerifiedFrameReceiver*> receiversOf(SnapshotDonor& donor, IVerifiedFrameReceiver* host)
{
    std::vector<IVerifiedFrameReceiver*> receivers{&donor};

    if (host != nullptr)
    {
        receivers.push_back(host);
    }

    return receivers;
}

}

NetworkedSession::NetworkedSession(sim::Frame& frame,
                                   const sim::SystemPipeline& pipeline,
                                   const net::SessionConfig& config,
                                   net::ITransport& transport,
                                   net::PeerId relay,
                                   std::uint32_t inputDelayFrames,
                                   IVerifiedFrameReceiver* receiver)
    : frame{frame}, pipeline{pipeline}, config{config}, transport{transport}, relay{relay},
      inputDelay{inputDelayFrames}, outbox{transport}, donor{outbox, relay},
      verifiedFrames{receiversOf(donor, receiver)}, pace{config.tickRate}
{
}

void NetworkedSession::join(std::uint64_t reconnectToken)
{
    outbox.send(
        relay, net::Channel::Reliable, net::Hello{net::kProtocolVersion, config, net::Role::Player, reconnectToken});

    connection.moveTo(ConnectionState::Connecting);
}

void NetworkedSession::spectate(std::uint32_t delayFrames)
{
    spectating = Spectating{delayFrames};
    outbox.send(relay, net::Channel::Reliable, net::Hello{net::kProtocolVersion, config, net::Role::Spectator, 0});
    connection.moveTo(ConnectionState::Connecting);
}

void NetworkedSession::setLocalInput(std::span<const std::byte> input)
{
    UNISON_VERIFY(input.size() <= localInput.size());

    if (input.size() > localInput.size())
    {
        return;
    }

    std::ranges::copy(input, localInput.begin());
}

void NetworkedSession::update(std::uint64_t now)
{
    updatedAt = now;
    transport.poll(*this);

    if (connection.isInMatch() && now >= nextPingAt)
    {
        outbox.send(relay, net::Channel::Unreliable, net::Ping{now});
        nextPingAt = now + kPingIntervalMicroseconds;
    }
}

void NetworkedSession::tick()
{
    if (!connection.isInMatch())
    {
        return;
    }

    played->setLocalInput(localInput);
    played->tick();
    connection.moveTo(played->isStalled() ? ConnectionState::Stalled : ConnectionState::Playing);
    catchUp.update(played->predictedFrame());

    if (!catchUp.isBehind())
    {
        sendInputs();
    }

    sendChecksums();
}

std::int32_t NetworkedSession::takeTickCorrection()
{
    if (played.has_value() && spectating.has_value())
    {
        return spectatorTickCorrection(catchUp.newestHeard(), spectating->delayFrames, played->predictedFrame());
    }

    const std::optional<std::int32_t> extraTicks =
        played.has_value() ? catchUp.extraTicks(played->predictedFrame()) : std::nullopt;

    return extraTicks.has_value() ? *extraTicks : pace.takeCorrection(updatedAt);
}

void NetworkedSession::receive(net::PeerId from, net::Channel, std::span<const std::byte> message)
{
    if (from != relay)
    {
        return;
    }

    const tl::expected<net::Message, Error> decoded = net::decode(message);

    if (!decoded.has_value())
    {
        return;
    }

    std::visit([this](const auto& received) { handle(received); }, *decoded);
}

void NetworkedSession::peerArrived(net::PeerId peer)
{
    if (peer == relay && connection.current() == ConnectionState::Connecting)
    {
        connection.moveTo(ConnectionState::Joining);
    }
}

void NetworkedSession::peerLeft(net::PeerId peer)
{
    if (peer == relay && connection.current() != ConnectionState::Idle)
    {
        connection.moveTo(ConnectionState::Disconnected);
    }
}

ConnectionState NetworkedSession::state() const
{
    return connection.current();
}

std::span<const ConnectionState> NetworkedSession::connectionChanges() const
{
    return connection.changes();
}

void NetworkedSession::clearConnectionChanges()
{
    connection.clearChanges();
}

std::uint8_t NetworkedSession::localSlot() const
{
    return welcomed.has_value() ? welcomed->slot : net::kNoSlot;
}

std::uint32_t NetworkedSession::startFrame() const
{
    return welcomed.has_value() ? welcomed->startFrame : 0;
}

std::uint64_t NetworkedSession::reconnectToken() const
{
    return welcomed.has_value() ? welcomed->reconnectToken : 0;
}

const Session* NetworkedSession::session() const
{
    return played.has_value() ? &*played : nullptr;
}

void NetworkedSession::clearEventChanges()
{
    if (played.has_value())
    {
        played->clearEventChanges();
    }
}

const TimeSync& NetworkedSession::timeSync() const
{
    return pace;
}

std::optional<net::Desync> NetworkedSession::lastDesync() const
{
    return reportedDesync;
}

void NetworkedSession::handle(const net::Welcome& welcome)
{
    const bool isWaitingToBeLetIn =
        connection.current() == ConnectionState::Connecting || connection.current() == ConnectionState::Joining;

    const bool isSeatOfItsRole =
        spectating.has_value() ? welcome.slot == net::kNoSlot : welcome.slot < config.slotCount;

    if (!isWaitingToBeLetIn || !isSeatOfItsRole)
    {
        return;
    }

    welcomed = welcome;
    catchUp.hear(welcome.confirmedFrame);

    if (welcome.startFrame > 0)
    {
        awaitedSnapshot.emplace(welcome.startFrame);

        return;
    }

    startSession();
    connection.moveTo(ConnectionState::Playing);
}

void NetworkedSession::handle(const net::SnapshotChunk& chunk)
{
    if (!awaitedSnapshot.has_value())
    {
        return;
    }

    const std::optional<tl::expected<sim::FrameSnapshot, Error>> snapshot = awaitedSnapshot->take(chunk);

    if (!snapshot.has_value())
    {
        return;
    }

    awaitedSnapshot.reset();
    startFrom(*snapshot);
}

void NetworkedSession::startFrom(const tl::expected<sim::FrameSnapshot, Error>& snapshot)
{
    if (!snapshot.has_value())
    {
        connection.moveTo(ConnectionState::Disconnected);

        return;
    }

    sim::restoreSnapshot(*snapshot, frame);
    startSession();
    catchUp.start();
    connection.moveTo(ConnectionState::Playing);
}

void NetworkedSession::startSession()
{
    if (spectating.has_value())
    {
        played.emplace(frame, pipeline, config, *spectating, &verifiedFrames);

        return;
    }

    played.emplace(frame, pipeline, config, welcomed->slot, inputDelay, &verifiedFrames);
}

void NetworkedSession::handle(const net::Confirmed& confirmed)
{
    if (confirmed.slotCount != config.slotCount || confirmed.inputSize != config.inputSize)
    {
        return;
    }

    if (confirmed.frameCount > 0)
    {
        catchUp.hear(confirmed.firstFrame + confirmed.frameCount - 1U);
    }

    if (!connection.isInMatch())
    {
        return;
    }

    const std::size_t frameSize = net::confirmedFrameSize(config.slotCount, config.inputSize);

    for (std::uint32_t offset = 0; offset < confirmed.frameCount; ++offset)
    {
        const std::span<const std::byte> slots = confirmed.slots.subspan(offset * frameSize, frameSize);

        static_cast<void>(played->confirm(confirmed.firstFrame + offset,
                                          inputsOfConfirmedFrame(slots, config.slotCount, config.inputSize)));
    }
}

void NetworkedSession::handle(const net::Pong& pong)
{
    if (!connection.isInMatch())
    {
        return;
    }

    pace.observe(pong, updatedAt, played->predictedFrame());
}

void NetworkedSession::handle(const net::Kick&)
{
    connection.moveTo(ConnectionState::Disconnected);
}

void NetworkedSession::handle(const net::Desync& desync)
{
    reportedDesync = desync;
}

void NetworkedSession::handle(const net::SnapshotRequest& request)
{
    if (connection.isInMatch())
    {
        donor.request(request.frame);
    }
}

void NetworkedSession::sendInputs()
{
    if (welcomed->slot == net::kNoSlot)
    {
        return;
    }

    const std::optional<net::Input> newest =
        newestInputsOf(*played, welcomed->slot, inputDelay, config.inputSize, inputBatch);

    if (newest.has_value())
    {
        outbox.send(relay, net::Channel::Unreliable, *newest);
    }
}

void NetworkedSession::sendChecksums()
{
    for (const VerifiedChecksum& verified : played->verifiedChecksums())
    {
        outbox.send(relay, net::Channel::Reliable, net::Checksum{verified.frameNumber, verified.checksum});
    }

    played->clearVerifiedChecksums();
}

}
