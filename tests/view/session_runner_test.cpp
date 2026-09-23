#include <unison/view/session_runner.hpp>

#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/outbox.hpp>
#include <unison/net/relay_core.hpp>

#include <support/fatal_handler_probe.hpp>
#include <support/session_script.hpp>
#include <unison/session/networked_session.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace
{

constexpr std::uint64_t kHostFrame = 16'700;
constexpr std::uint8_t kLocalSlot = 0;

unison::net::SessionConfig twoPlayers()
{
    unison::net::SessionConfig config;
    config.slotCount = 2;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = 8;

    return config;
}

struct Rig
{
    Rig()
    {
        unison::test::addScoredEntity(frame);
        pipeline.add(mixer);
        pipeline.add(moves);
        client.join();
        otherOutbox.send(relayEnd.id(),
                         unison::net::Channel::Reliable,
                         unison::net::Hello{
                             unison::net::kProtocolVersion, unison::net::hashOf(config), unison::net::Role::Player, 0});
        relayEnd.poll(relay);
    }

    unison::view::RunnerStep step(std::uint64_t hostDelta)
    {
        clock.advance(hostDelta);
        runner.setLocalInput(unison::test::bytesOf(unison::test::inputWithMove(1)));

        const unison::view::RunnerStep ran = runner.update(hostDelta);

        keepTheOtherPlayerAlongside();
        relayEnd.poll(relay);
        relay.update();

        return ran;
    }

    void keepTheOtherPlayerAlongside()
    {
        if (client.session() == nullptr)
        {
            return;
        }

        sendOtherInputs(client.session()->predictedFrame(), 1);
    }

    void sendOtherInputs(std::uint32_t firstFrame, std::uint8_t frames)
    {
        otherInputs.fill(std::byte{0});
        otherOutbox.send(relayEnd.id(),
                         unison::net::Channel::Unreliable,
                         unison::net::Input{firstFrame,
                                            config.inputSize,
                                            frames,
                                            std::span{otherInputs}.first(std::size_t{frames} * config.inputSize)});
    }

    unison::net::SessionConfig config = twoPlayers();
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& relayEnd = hub.join();
    unison::net::LoopbackEndpoint& clientEnd = hub.join();
    unison::net::LoopbackEndpoint& otherEnd = hub.join();
    unison::net::Outbox otherOutbox{otherEnd};
    unison::net::ManualClock clock;
    unison::net::RelayCore relay{relayEnd, clock, config};
    unison::sim::Frame frame;
    unison::test::InputMixer mixer;
    unison::test::MoveAnnouncer moves;
    unison::sim::SystemPipeline pipeline;
    unison::session::NetworkedSession client{frame, pipeline, config, clientEnd, relayEnd.id()};
    unison::view::EventDispatcher dispatcher;
    unison::view::SessionRunner runner{client, dispatcher, clock, config.tickRate};
    std::array<std::byte, 64U * sizeof(unison::test::SampleInput)> otherInputs{};
};

}

TEST_CASE("every 16.7 ms of host time runs one tick")
{
    Rig rig;
    std::uint32_t ticks = 0;
    std::uint32_t framesRunningOne = 0;

    for (std::uint32_t hostFrame = 0; hostFrame < 10; ++hostFrame)
    {
        const std::uint32_t ran = rig.step(kHostFrame).ticks;

        ticks += ran;
        framesRunningOne += ran == 1U ? 1U : 0U;
    }

    REQUIRE(ticks == 10U);
    REQUIRE(framesRunningOne == 10U);
    REQUIRE(rig.client.session()->predictedFrame() == 10U);
}

TEST_CASE("50 ms of host time runs three ticks")
{
    Rig rig;

    REQUIRE(rig.step(50'000).ticks == 3U);
}

TEST_CASE("host time shorter than a tick is carried over to the next host frame")
{
    Rig rig;

    const std::uint32_t first = rig.step(10'000).ticks;
    const std::uint32_t second = rig.step(10'000).ticks;

    REQUIRE(first == 0U);
    REQUIRE(second == 1U);
}

TEST_CASE("the alpha is the share of a tick the host's time has gone past the last one")
{
    Rig rig;

    const unison::view::RunnerStep ran = rig.step(25'000);

    REQUIRE(ran.ticks == 1U);
    REQUIRE_THAT(ran.alpha, Catch::Matchers::WithinAbs(0.5, 0.01));
}

TEST_CASE("the events the ticks raise reach the host's handlers once each")
{
    Rig rig;
    std::uint32_t moved = 0;
    rig.dispatcher.on<unison::test::SlotMoved>([&moved](const unison::sim::EventKey&, const unison::test::SlotMoved&)
                                               { ++moved; });

    for (std::uint32_t hostFrame = 0; hostFrame < 5; ++hostFrame)
    {
        static_cast<void>(rig.step(kHostFrame));
    }

    REQUIRE(moved == 5U);
}

TEST_CASE("the dispatcher forgets the events of the frames the session has verified")
{
    Rig rig;
    std::uint32_t moved = 0;
    rig.dispatcher.on<unison::test::SlotMoved>([&moved](const unison::sim::EventKey&, const unison::test::SlotMoved&)
                                               { ++moved; });

    for (std::uint32_t hostFrame = 0; hostFrame < 20; ++hostFrame)
    {
        static_cast<void>(rig.step(kHostFrame));
    }

    unison::session::EventChanges firstFrameAgain;
    firstFrameAgain.raised.raise(0, unison::test::SlotMoved{kLocalSlot});
    rig.dispatcher.dispatch(firstFrameAgain);

    REQUIRE(rig.client.session()->verifiedFrame() > 1U);
    REQUIRE(moved == 21U);
}

TEST_CASE("a session behind the relay's clock runs an extra tick in a host frame until it catches up")
{
    Rig rig;
    rig.sendOtherInputs(1, 60);
    rig.relayEnd.poll(rig.relay);

    std::uint32_t mostTicksInAFrame = 0;

    for (std::uint32_t hostFrame = 0; hostFrame < 60; ++hostFrame)
    {
        const std::uint32_t ran = rig.step(kHostFrame).ticks;

        mostTicksInAFrame = ran > mostTicksInAFrame ? ran : mostTicksInAFrame;
    }

    REQUIRE(mostTicksInAFrame == 2U);
}

TEST_CASE("a runner of a match that never ticks breaks a contract")
{
    Rig rig;
    const unison::test::FatalHandlerProbe probe;

    const unison::view::SessionRunner runner{rig.client, rig.dispatcher, rig.clock, 0};

    REQUIRE(probe.failureCount() == 1U);
}
