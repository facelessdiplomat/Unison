#include <unison/session/networked_session.hpp>

#include <unison/session/snapshot_chunks.hpp>
#include <unison/session/snapshot_serializer.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <unison/net/loopback_hub.hpp>
#include <unison/net/message_codec.hpp>
#include <unison/net/outbox.hpp>

#include <support/fatal_handler_probe.hpp>
#include <support/session_script.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

namespace
{

using unison::session::ConnectionState;
using unison::test::SampleInput;

constexpr std::uint8_t kLocalSlot = unison::test::kSessionLocalSlot;
constexpr std::uint64_t kTickMicroseconds = 16'667;

unison::net::SessionConfig threePlayers()
{
    unison::net::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(SampleInput);
    config.maxPrediction = 8;
    config.checksumInterval = 1;

    return config;
}

class Mailbox final : public unison::net::IMessageReceiver
{
public:
    void receive(unison::net::PeerId, unison::net::Channel channel, std::span<const std::byte> message) override
    {
        channels.push_back(channel);
        letters.emplace_back(message.begin(), message.end());
    }

    template <typename T>
    [[nodiscard]] std::vector<T> all() const
    {
        std::vector<T> found;

        for (const std::vector<std::byte>& letter : letters)
        {
            const auto decoded = unison::net::decode(letter);

            if (decoded.has_value() && std::holds_alternative<T>(*decoded))
            {
                found.push_back(std::get<T>(*decoded));
            }
        }

        return found;
    }

    std::vector<unison::net::Channel> channels;
    std::vector<std::vector<std::byte>> letters;
};

struct Rig
{
    Rig()
    {
        unison::test::addScoredEntity(frame);
        pipeline.add(mixer);
    }

    void welcome(std::uint8_t slot)
    {
        relayOutbox.send(clientEnd.id(), unison::net::Channel::Reliable, unison::net::Welcome{slot, config, 0, 0, 0});
    }

    void confirm(std::uint32_t frameNumber, const unison::sim::FrameInputs& inputs)
    {
        confirmInOneMessage(frameNumber, std::vector<unison::sim::FrameInputs>{inputs});
    }

    void confirmInOneMessage(std::uint32_t firstFrame, const std::vector<unison::sim::FrameInputs>& frames)
    {
        const std::size_t stride = 1U + config.inputSize;
        const std::size_t frameSize = config.slotCount * stride;
        std::vector<std::byte> slots(frames.size() * frameSize);

        for (std::size_t index = 0; index < frames.size(); ++index)
        {
            for (std::size_t slot = 0; slot < config.slotCount; ++slot)
            {
                const std::size_t at = index * frameSize + slot * stride;
                const auto input = frames[index].bytesAt(slot).first(config.inputSize);

                slots[at] = static_cast<std::byte>(frames[index].flagsAt(slot));
                std::copy(input.begin(), input.end(), slots.begin() + static_cast<std::ptrdiff_t>(at + 1U));
            }
        }

        relayOutbox.send(
            clientEnd.id(),
            unison::net::Channel::Unreliable,
            unison::net::Confirmed{
                firstFrame, config.slotCount, config.inputSize, static_cast<std::uint8_t>(frames.size()), slots});
    }

    void playWithMove(std::int8_t moveX)
    {
        client.setLocalInput(unison::test::bytesOf(unison::test::inputWithMove(moveX)));
        client.update(now);
        client.tick();
        now += kTickMicroseconds;
    }

    void pongAt(std::uint64_t sentAt, std::uint32_t dueFrame)
    {
        relayOutbox.send(clientEnd.id(), unison::net::Channel::Unreliable, unison::net::Pong{sentAt, 0, dueFrame});
    }

    Mailbox& relayMail()
    {
        relayEnd.poll(atRelay);

        return atRelay;
    }

    unison::net::SessionConfig config = threePlayers();
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& relayEnd = hub.join();
    unison::net::LoopbackEndpoint& clientEnd = hub.join();
    unison::net::Outbox relayOutbox{relayEnd};
    unison::sim::Frame frame;
    unison::test::InputMixer mixer;
    unison::sim::SystemPipeline pipeline;
    unison::session::NetworkedSession client{frame, pipeline, config, clientEnd, relayEnd.id()};
    Mailbox atRelay;
    std::uint64_t now = 0;
};

std::vector<std::int8_t> movesOf(const unison::net::Input& input)
{
    std::vector<std::int8_t> moves;

    for (std::size_t frame = 0; frame < input.frameCount; ++frame)
    {
        const auto bytes = input.inputs.subspan(frame * input.inputSize, input.inputSize);
        moves.push_back(static_cast<std::int8_t>(bytes.front()));
    }

    return moves;
}

}

TEST_CASE("a new networked session has not asked to join yet")
{
    Rig rig;

    rig.playWithMove(1);

    REQUIRE(rig.client.state() == ConnectionState::Idle);
    REQUIRE(rig.relayMail().letters.empty());
}

TEST_CASE("joining says hello to the relay as a player of the config the client plays")
{
    Rig rig;

    rig.client.join();

    const std::vector<unison::net::Hello> hellos = rig.relayMail().all<unison::net::Hello>();
    REQUIRE(hellos.size() == 1U);
    REQUIRE(hellos.front().protocolVersion == unison::net::kProtocolVersion);
    REQUIRE(unison::net::hashOf(hellos.front().config) == unison::net::hashOf(rig.config));
    REQUIRE(hellos.front().role == unison::net::Role::Player);
    REQUIRE(rig.atRelay.channels.front() == unison::net::Channel::Reliable);
    REQUIRE(rig.client.state() == ConnectionState::Connecting);
}

TEST_CASE("a connecting client is joining once its transport has reached the relay")
{
    Rig rig;
    rig.client.join();

    rig.client.peerArrived(rig.relayEnd.id());

    REQUIRE(rig.client.state() == ConnectionState::Joining);
}

TEST_CASE("a client plays nothing until the relay lets it in")
{
    Rig rig;
    rig.client.join();

    rig.playWithMove(1);
    rig.playWithMove(2);

    REQUIRE(rig.client.state() == ConnectionState::Connecting);
    REQUIRE(rig.client.session() == nullptr);
    REQUIRE(rig.client.localSlot() == unison::net::kNoSlot);
    REQUIRE(rig.relayMail().all<unison::net::Input>().empty());
    REQUIRE(rig.frame.frameNumber == 0U);
}

TEST_CASE("a client the relay welcomes into a slot plays in it")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);

    rig.playWithMove(3);

    REQUIRE(rig.client.state() == ConnectionState::Playing);
    REQUIRE(rig.client.localSlot() == kLocalSlot);
    REQUIRE(rig.client.session()->predictedFrame() == 1U);
    REQUIRE(rig.client.session()->inputs().inputsAt(1).get<SampleInput>(kLocalSlot).moveX == 3);
}

TEST_CASE("a welcome into a slot the match does not have is ignored")
{
    Rig rig;
    rig.client.join();
    rig.welcome(unison::net::kNoSlot);

    rig.playWithMove(1);

    REQUIRE(rig.client.state() == ConnectionState::Connecting);
}

TEST_CASE("a client sends the input of every frame it plays with the three before it")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);

    for (std::int8_t move = 1; move <= 6; ++move)
    {
        rig.playWithMove(move);
    }

    const std::vector<unison::net::Input> inputs = rig.relayMail().all<unison::net::Input>();
    REQUIRE(inputs.size() == 6U);
    REQUIRE(inputs.front().firstFrame == 1U);
    REQUIRE(movesOf(inputs.front()) == std::vector<std::int8_t>{1});
    REQUIRE(inputs.back().firstFrame == 3U);
    REQUIRE(inputs.back().inputSize == sizeof(SampleInput));
    REQUIRE(movesOf(inputs.back()) == std::vector<std::int8_t>{3, 4, 5, 6});
    REQUIRE(rig.atRelay.channels.back() == unison::net::Channel::Unreliable);
}

TEST_CASE("a client stops sending the inputs of frames it has verified")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);
    rig.playWithMove(2);
    rig.confirm(1, rig.client.session()->inputs().inputsAt(1));
    rig.confirm(2, rig.client.session()->inputs().inputsAt(2));

    rig.playWithMove(3);

    const std::vector<unison::net::Input> inputs = rig.relayMail().all<unison::net::Input>();
    REQUIRE(inputs.back().firstFrame == 3U);
    REQUIRE(movesOf(inputs.back()) == std::vector<std::int8_t>{3});
}

TEST_CASE("a frame the relay confirms is verified")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);
    rig.confirm(1, unison::test::scriptedSessionInputs(1));

    rig.playWithMove(2);

    REQUIRE(rig.client.session()->verifiedFrame() == 1U);
}

TEST_CASE("a client settles a frame whose own confirmation was lost from the one that came after it")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);
    rig.playWithMove(2);
    rig.confirmInOneMessage(1, {unison::test::scriptedSessionInputs(1), unison::test::scriptedSessionInputs(2)});

    rig.playWithMove(3);

    REQUIRE(rig.client.session()->verifiedFrame() == 2U);
}

TEST_CASE("frames a confirmation repeats that the client has settled already change nothing")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);
    rig.playWithMove(2);
    const unison::sim::FrameInputs first = rig.client.session()->inputs().inputsAt(1);
    const unison::sim::FrameInputs second = rig.client.session()->inputs().inputsAt(2);
    rig.confirm(1, first);
    rig.playWithMove(3);
    const unison::session::RollbackStats before = rig.client.session()->rollbackStats();
    rig.confirmInOneMessage(1, {first, second});

    rig.playWithMove(4);

    REQUIRE(rig.client.session()->verifiedFrame() == 2U);
    REQUIRE(rig.client.session()->rollbackStats().rollbacks == before.rollbacks);
}

TEST_CASE("a client reports the checksum of every frame it verifies on the interval")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);
    rig.playWithMove(2);
    rig.confirm(1, rig.client.session()->inputs().inputsAt(1));
    rig.confirm(2, rig.client.session()->inputs().inputsAt(2));

    rig.playWithMove(3);

    const std::vector<unison::net::Checksum> checksums = rig.relayMail().all<unison::net::Checksum>();
    REQUIRE(checksums.size() == 2U);
    REQUIRE(checksums[0].frame == 1U);
    REQUIRE(checksums[1].frame == 2U);
    REQUIRE(checksums[0].checksum != checksums[1].checksum);
}

TEST_CASE("a client the relay sends away stops playing")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);
    rig.relayOutbox.send(
        rig.clientEnd.id(), unison::net::Channel::Reliable, unison::net::Kick{unison::net::LeaveReason::RoomFull});

    rig.playWithMove(2);
    rig.playWithMove(3);

    REQUIRE(rig.client.state() == ConnectionState::Disconnected);
    REQUIRE(rig.client.session()->predictedFrame() == 1U);
}

TEST_CASE("a playing client whose prediction window is full is stalled until a frame is confirmed")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);

    for (std::int8_t move = 0; move <= static_cast<std::int8_t>(rig.config.maxPrediction); ++move)
    {
        rig.playWithMove(move);
    }

    const ConnectionState whenFull = rig.client.state();
    rig.confirm(1, rig.client.session()->inputs().inputsAt(1));
    rig.playWithMove(1);

    REQUIRE(whenFull == ConnectionState::Stalled);
    REQUIRE(rig.client.state() == ConnectionState::Playing);
}

TEST_CASE("a client whose relay has gone is disconnected")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);

    rig.client.peerLeft(rig.relayEnd.id());

    REQUIRE(rig.client.state() == ConnectionState::Disconnected);
}

TEST_CASE("peers other than the relay coming and going change nothing for a client")
{
    Rig rig;
    rig.client.join();

    rig.client.peerArrived(unison::net::PeerId{77});
    rig.client.peerLeft(unison::net::PeerId{77});

    REQUIRE(rig.client.state() == ConnectionState::Connecting);
}

TEST_CASE("every state a client moves into is kept for the host, in order, until the host clears them")
{
    Rig rig;
    rig.client.join();
    rig.client.peerArrived(rig.relayEnd.id());
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);
    const std::vector<ConnectionState> moved{rig.client.connectionChanges().begin(),
                                             rig.client.connectionChanges().end()};

    rig.client.clearConnectionChanges();

    REQUIRE(moved == std::vector<ConnectionState>{
                         ConnectionState::Connecting, ConnectionState::Joining, ConnectionState::Playing});
    REQUIRE(rig.client.connectionChanges().empty());
}

TEST_CASE("a client keeps the desync the relay reports")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.relayOutbox.send(rig.clientEnd.id(), unison::net::Channel::Reliable, unison::net::Desync{40, 0b010U});

    rig.playWithMove(1);

    REQUIRE(rig.client.lastDesync().has_value());
    REQUIRE(rig.client.lastDesync()->frame == 40U);
    REQUIRE(rig.client.lastDesync()->minoritySlots == 0b010U);
}

TEST_CASE("a client listens to the relay and to nobody else")
{
    Rig rig;
    unison::net::LoopbackEndpoint& stranger = rig.hub.join();
    unison::net::Outbox strangerOutbox{stranger};
    rig.client.join();
    strangerOutbox.send(
        rig.clientEnd.id(), unison::net::Channel::Reliable, unison::net::Welcome{kLocalSlot, rig.config, 0, 0, 0});

    rig.playWithMove(1);

    REQUIRE(rig.client.state() == ConnectionState::Connecting);
}

TEST_CASE("giving a networked session a local input that does not fit a slot breaks a contract")
{
    Rig rig;
    const std::array<std::byte, unison::sim::kMaxInputSize + 1> tooLong{};
    const unison::test::FatalHandlerProbe probe;

    rig.client.setLocalInput(tooLong);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a playing client pings the relay with the time every hundred milliseconds")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);

    for (std::uint64_t at = 0; at <= 200'000; at += 50'000)
    {
        rig.client.update(at);
    }

    const std::vector<unison::net::Ping> pings = rig.relayMail().all<unison::net::Ping>();
    REQUIRE(pings.size() == 3U);
    REQUIRE(pings[0].sentAt == 0U);
    REQUIRE(pings[1].sentAt == 100'000U);
    REQUIRE(pings[2].sentAt == 200'000U);
}

TEST_CASE("a client the relay's pongs show running ahead of the relay's clock is told to run a tick fewer")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);

    for (std::int8_t move = 0; move < 10; ++move)
    {
        rig.playWithMove(move);
    }

    for (std::uint32_t pong = 0; pong < unison::session::TimeSyncSettings{}.pongsPerJudgement; ++pong)
    {
        rig.pongAt(rig.now, 1);
    }

    rig.client.update(rig.now);

    REQUIRE(rig.client.takeTickCorrection() == -1);
}

TEST_CASE("a pong tells the client how long the round trip to the relay took")
{
    Rig rig;
    rig.now = 1'000'000;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.playWithMove(1);
    rig.pongAt(rig.now - 40'000, 1);

    rig.client.update(rig.now);

    REQUIRE(rig.client.timeSync().roundTripMicroseconds() == 40'000U);
}

TEST_CASE("a client asked for a snapshot sends the first frame it verifies from then on in chunks of that frame")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);
    rig.relayOutbox.send(rig.clientEnd.id(), unison::net::Channel::Reliable, unison::net::SnapshotRequest{2});

    for (std::uint32_t frame = 1; frame <= 3; ++frame)
    {
        rig.confirm(frame, unison::test::scriptedSessionInputs(frame));
        rig.playWithMove(unison::test::scriptedSessionInput(frame, kLocalSlot).moveX);
    }

    const Mailbox& mail = rig.relayMail();
    unison::session::SnapshotAssembler assembler;

    for (const unison::net::SnapshotChunk& chunk : mail.all<unison::net::SnapshotChunk>())
    {
        REQUIRE(assembler.add(chunk));
    }

    REQUIRE(assembler.isComplete());
    REQUIRE(assembler.frame() == 2U);
    unison::sim::FrameSnapshot snapshot;
    REQUIRE(unison::session::deserializeSnapshot(assembler.bytes(), snapshot).has_value());
    const std::vector<unison::net::Checksum> checksums = mail.all<unison::net::Checksum>();
    const auto reported = std::ranges::find(checksums, 2U, &unison::net::Checksum::frame);
    REQUIRE(reported != checksums.end());
    REQUIRE(unison::sim::checksumOf(snapshot) == reported->checksum);
}

namespace
{

constexpr std::uint32_t kSnapshotFrame = 5;

std::vector<std::byte> scriptedSnapshotAt(std::uint32_t frames)
{
    unison::sim::Frame frame;
    unison::test::addScoredEntity(frame);
    unison::test::InputMixer mixer;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(mixer);

    for (std::uint32_t next = 1; next <= frames; ++next)
    {
        unison::sim::advanceFrame(frame, pipeline, unison::test::scriptedSessionInputs(next));
    }

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);
    std::vector<std::byte> bytes;
    unison::session::serializeSnapshot(snapshot, bytes);

    return bytes;
}

void handSnapshot(Rig& rig, std::uint32_t confirmedFrame, const std::vector<std::byte>& snapshot)
{
    rig.relayOutbox.send(rig.clientEnd.id(),
                         unison::net::Channel::Reliable,
                         unison::net::Welcome{kLocalSlot, rig.config, kSnapshotFrame, confirmedFrame, 0});

    for (const unison::net::SnapshotChunk& chunk : unison::session::chunksOf(kSnapshotFrame, snapshot))
    {
        rig.relayOutbox.send(rig.clientEnd.id(), unison::net::Channel::Reliable, chunk);
    }
}

void welcomeLate(Rig& rig, std::uint32_t confirmedFrame, const std::vector<std::byte>& snapshot)
{
    rig.client.join();
    handSnapshot(rig, confirmedFrame, snapshot);

    for (std::uint32_t frame = kSnapshotFrame + 1; frame <= confirmedFrame; ++frame)
    {
        rig.confirm(frame, unison::test::scriptedSessionInputs(frame));
    }
}

}

TEST_CASE("a late joiner restores the snapshot it is welcomed at and plays on to the checksums of the others")
{
    Rig rig;
    const std::vector<std::byte> snapshot = scriptedSnapshotAt(kSnapshotFrame);
    welcomeLate(rig, kSnapshotFrame + 3, snapshot);

    for (std::uint32_t tick = 0; tick < 3; ++tick)
    {
        rig.playWithMove(0);
    }

    REQUIRE(rig.client.state() == ConnectionState::Playing);
    const std::vector<unison::net::Checksum> checksums = rig.relayMail().all<unison::net::Checksum>();
    REQUIRE(checksums.size() == 3U);

    for (const unison::net::Checksum& checksum : checksums)
    {
        CAPTURE(checksum.frame);
        REQUIRE(checksum.checksum == unison::test::checksumOfScriptedSession(checksum.frame));
    }
}

TEST_CASE("a late joiner catches up at no more than eight frames a host frame")
{
    Rig rig;
    welcomeLate(rig, kSnapshotFrame + 20, scriptedSnapshotAt(kSnapshotFrame));
    rig.client.update(rig.now);

    const std::int32_t farBehind = rig.client.takeTickCorrection();
    for (std::uint32_t tick = 0; tick < 17; ++tick)
    {
        rig.client.tick();
    }
    const std::int32_t nearlyCaughtUp = rig.client.takeTickCorrection();

    REQUIRE(farBehind == 7);
    REQUIRE(nearlyCaughtUp == 3);
}

TEST_CASE("a late joiner sends no input until it has caught up")
{
    Rig rig;
    rig.client.join();
    handSnapshot(rig, kSnapshotFrame + 3, scriptedSnapshotAt(kSnapshotFrame));

    rig.playWithMove(0);
    rig.playWithMove(0);
    const std::vector<unison::net::Input> whileBehind = rig.relayMail().all<unison::net::Input>();
    for (std::uint32_t frame = kSnapshotFrame + 1; frame <= kSnapshotFrame + 3; ++frame)
    {
        rig.confirm(frame, unison::test::scriptedSessionInputs(frame));
    }
    rig.playWithMove(0);
    rig.playWithMove(0);
    const std::vector<unison::net::Input> caughtUp = rig.relayMail().all<unison::net::Input>();

    REQUIRE(whileBehind.empty());
    REQUIRE_FALSE(caughtUp.empty());
}

TEST_CASE("a late joiner catches up to the newest frame it has heard confirmed, though its welcome names an older one")
{
    Rig rig;
    rig.client.join();
    rig.confirm(kSnapshotFrame + 20, unison::test::scriptedSessionInputs(kSnapshotFrame + 20));
    handSnapshot(rig, kSnapshotFrame + 1, scriptedSnapshotAt(kSnapshotFrame));

    rig.client.update(rig.now);

    REQUIRE(rig.client.takeTickCorrection() == 7);
}

TEST_CASE("a late joiner handed the snapshot of another frame than its welcome named is disconnected")
{
    Rig rig;
    rig.client.join();
    rig.relayOutbox.send(rig.clientEnd.id(),
                         unison::net::Channel::Reliable,
                         unison::net::Welcome{kLocalSlot, rig.config, kSnapshotFrame, kSnapshotFrame, 0});
    const std::vector<std::byte> later = scriptedSnapshotAt(kSnapshotFrame + 1);

    for (const unison::net::SnapshotChunk& chunk : unison::session::chunksOf(kSnapshotFrame + 1, later))
    {
        rig.relayOutbox.send(rig.clientEnd.id(), unison::net::Channel::Reliable, chunk);
    }
    rig.client.update(rig.now);

    REQUIRE(rig.client.state() == ConnectionState::Disconnected);
    REQUIRE(rig.client.session() == nullptr);
}

TEST_CASE("a late joiner handed bytes that are no snapshot is disconnected")
{
    Rig rig;
    rig.client.join();
    handSnapshot(rig, kSnapshotFrame, std::vector<std::byte>(16, std::byte{7}));

    rig.client.update(rig.now);

    REQUIRE(rig.client.state() == ConnectionState::Disconnected);
    REQUIRE(rig.client.session() == nullptr);
}

TEST_CASE("a client let in at the start of a match starts from frame nought")
{
    Rig rig;
    rig.client.join();
    rig.welcome(kLocalSlot);

    rig.playWithMove(0);

    REQUIRE(rig.client.startFrame() == 0U);
}

TEST_CASE("a late joiner starts from the frame of the snapshot it is welcomed at")
{
    Rig rig;
    welcomeLate(rig, kSnapshotFrame + 3, scriptedSnapshotAt(kSnapshotFrame));

    rig.playWithMove(0);

    REQUIRE(rig.client.startFrame() == kSnapshotFrame);
}

TEST_CASE("a late joiner welcomed again before its snapshot is whole restores the snapshot of the newer welcome")
{
    Rig rig;
    rig.client.join();
    rig.relayOutbox.send(rig.clientEnd.id(),
                         unison::net::Channel::Reliable,
                         unison::net::Welcome{kLocalSlot, rig.config, kSnapshotFrame - 1, kSnapshotFrame, 0});
    const std::vector<std::byte> unfinished(2 * unison::net::snapshotBytesPerChunk(), std::byte{1});
    rig.relayOutbox.send(rig.clientEnd.id(),
                         unison::net::Channel::Reliable,
                         unison::session::chunksOf(kSnapshotFrame - 1, unfinished).front());
    handSnapshot(rig, kSnapshotFrame, scriptedSnapshotAt(kSnapshotFrame));

    rig.client.update(rig.now);

    REQUIRE(rig.client.state() == ConnectionState::Playing);
    REQUIRE(rig.client.startFrame() == kSnapshotFrame);
}

TEST_CASE("a client keeps the reconnect token its welcome carried")
{
    Rig rig;
    rig.client.join();
    rig.relayOutbox.send(
        rig.clientEnd.id(), unison::net::Channel::Reliable, unison::net::Welcome{kLocalSlot, rig.config, 0, 0, 77});

    rig.playWithMove(0);

    REQUIRE(rig.client.reconnectToken() == 77U);
}

TEST_CASE("a client joining with a reconnect token asks for its slot back with it")
{
    Rig rig;

    rig.client.join(77);

    const std::vector<unison::net::Hello> hellos = rig.relayMail().all<unison::net::Hello>();
    REQUIRE(hellos.size() == 1U);
    REQUIRE(hellos.front().reconnectToken == 77U);
}
