#include <unison/session/networked_session.hpp>

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
        const std::size_t stride = 1U + config.inputSize;
        std::vector<std::byte> slots(config.slotCount * stride);

        for (std::size_t slot = 0; slot < config.slotCount; ++slot)
        {
            slots[slot * stride] = static_cast<std::byte>(inputs.flagsAt(slot));

            const auto input = inputs.bytesAt(slot).first(config.inputSize);
            std::copy(input.begin(), input.end(), slots.begin() + static_cast<std::ptrdiff_t>(slot * stride + 1U));
        }

        relayOutbox.send(clientEnd.id(),
                         unison::net::Channel::Unreliable,
                         unison::net::Confirmed{frameNumber, config.slotCount, config.inputSize, slots});
    }

    void playWithMove(std::int8_t moveX)
    {
        client.setLocalInput(unison::test::bytesOf(unison::test::inputWithMove(moveX)));
        client.update(now);
        client.tick();
        now += kTickMicroseconds;
    }

    void pongAt(std::uint64_t sentAt, std::uint32_t confirmedFrame)
    {
        relayOutbox.send(clientEnd.id(), unison::net::Channel::Unreliable, unison::net::Pong{sentAt, confirmedFrame});
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
    REQUIRE(hellos.front().configHash == unison::net::hashOf(rig.config));
    REQUIRE(hellos.front().role == unison::net::Role::Player);
    REQUIRE(rig.atRelay.channels.front() == unison::net::Channel::Reliable);
    REQUIRE(rig.client.state() == ConnectionState::Joining);
}

TEST_CASE("a client plays nothing until the relay lets it in")
{
    Rig rig;
    rig.client.join();

    rig.playWithMove(1);
    rig.playWithMove(2);

    REQUIRE(rig.client.state() == ConnectionState::Joining);
    REQUIRE(rig.client.session() == nullptr);
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
    REQUIRE(rig.client.session()->predictedFrame() == 1U);
    REQUIRE(rig.client.session()->inputs().inputsAt(1).get<SampleInput>(kLocalSlot).moveX == 3);
}

TEST_CASE("a welcome into a slot the match does not have is ignored")
{
    Rig rig;
    rig.client.join();
    rig.welcome(unison::net::kNoSlot);

    rig.playWithMove(1);

    REQUIRE(rig.client.state() == ConnectionState::Joining);
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

    REQUIRE(rig.client.state() == ConnectionState::Joining);
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

TEST_CASE("a client the relay's pongs show running ahead is told to run a tick fewer")
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
        rig.pongAt(rig.now, 0);
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
    rig.pongAt(rig.now - 40'000, 0);

    rig.client.update(rig.now);

    REQUIRE(rig.client.timeSync().roundTripMicroseconds() == 40'000U);
}
