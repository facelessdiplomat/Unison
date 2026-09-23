#include <unison/session/networked_session.hpp>

#include <unison/core/contract.hpp>
#include <unison/net/message_codec.hpp>

#include <algorithm>
#include <variant>

namespace unison::session
{

NetworkedSession::NetworkedSession(sim::Frame& frame,
                                   const sim::SystemPipeline& pipeline,
                                   const net::SessionConfig& config,
                                   net::ITransport& transport,
                                   net::PeerId relay,
                                   std::uint32_t inputDelayFrames)
    : frame{frame}, pipeline{pipeline}, config{config}, transport{transport}, relay{relay},
      inputDelay{inputDelayFrames}, outbox{transport}, pace{config.tickRate}
{
}

void NetworkedSession::join()
{
    outbox.send(
        relay, net::Channel::Reliable, net::Hello{net::kProtocolVersion, net::hashOf(config), net::Role::Player, 0});

    connection = ConnectionState::Joining;
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

    if (connection == ConnectionState::Playing && now >= nextPingAt)
    {
        outbox.send(relay, net::Channel::Unreliable, net::Ping{now});
        nextPingAt = now + kPingIntervalMicroseconds;
    }
}

void NetworkedSession::tick()
{
    if (connection != ConnectionState::Playing)
    {
        return;
    }

    played->setLocalInput(localInput);
    played->tick();

    sendInputs();
    sendChecksums();
}

std::int32_t NetworkedSession::takeTickCorrection()
{
    return pace.takeCorrection(updatedAt);
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

ConnectionState NetworkedSession::state() const
{
    return connection;
}

std::uint8_t NetworkedSession::localSlot() const
{
    return givenSlot;
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
    if (connection != ConnectionState::Joining || welcome.slot >= config.slotCount)
    {
        return;
    }

    givenSlot = welcome.slot;
    played.emplace(frame, pipeline, config, givenSlot, inputDelay);
    connection = ConnectionState::Playing;
}

void NetworkedSession::handle(const net::Confirmed& confirmed)
{
    if (connection != ConnectionState::Playing || confirmed.slotCount != config.slotCount ||
        confirmed.inputSize != config.inputSize)
    {
        return;
    }

    const std::size_t frameSize = net::confirmedFrameSize(config.slotCount, config.inputSize);

    for (std::uint32_t offset = 0; offset < confirmed.frameCount; ++offset)
    {
        settle(confirmed.firstFrame + offset, confirmed.slots.subspan(offset * frameSize, frameSize));
    }
}

void NetworkedSession::handle(const net::Pong& pong)
{
    if (connection != ConnectionState::Playing)
    {
        return;
    }

    pace.observe(pong, updatedAt, played->predictedFrame());
}

void NetworkedSession::handle(const net::Kick&)
{
    connection = ConnectionState::Disconnected;
}

void NetworkedSession::handle(const net::Desync& desync)
{
    reportedDesync = desync;
}

void NetworkedSession::settle(std::uint32_t frameNumber, std::span<const std::byte> slots)
{
    const std::size_t stride = 1U + config.inputSize;
    sim::FrameInputs inputs;

    for (std::size_t slot = 0; slot < config.slotCount; ++slot)
    {
        const std::span<const std::byte> entry = slots.subspan(slot * stride, stride);

        inputs.setBytes(slot, entry.subspan(1), static_cast<sim::InputFlags>(std::to_integer<std::uint8_t>(entry[0])));
    }

    static_cast<void>(played->confirm(frameNumber, inputs));
}

void NetworkedSession::sendInputs()
{
    const std::uint32_t newest = played->predictedFrame() + inputDelay;
    const std::uint32_t oldestRepeated = newest < kRedundantInputs ? 1U : newest + 1U - kRedundantInputs;
    const std::uint32_t oldest = std::max(played->verifiedFrame() + 1U, oldestRepeated);

    if (newest < oldest)
    {
        return;
    }

    const std::uint32_t count = newest - oldest + 1U;
    const std::size_t inputSize = config.inputSize;
    const std::span<std::byte> batch = std::span{inputBatch}.first(count * inputSize);

    for (std::uint32_t offset = 0; offset < count; ++offset)
    {
        std::ranges::copy(played->inputs().inputsAt(oldest + offset).bytesAt(givenSlot).first(inputSize),
                          batch.subspan(offset * inputSize).begin());
    }

    outbox.send(
        relay, net::Channel::Unreliable, net::Input{oldest, config.inputSize, static_cast<std::uint8_t>(count), batch});
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
