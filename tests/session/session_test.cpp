#include <unison/session/session.hpp>

#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace
{

struct SampleInput
{
    std::int8_t moveX = 0;
    std::int8_t moveY = 0;
    std::int16_t yaw = 0;
    std::uint32_t buttons = 0;
};

constexpr std::size_t kLocalSlot = 1;

class InputRecorder final : public unison::sim::ISystem
{
public:
    void update(unison::sim::Frame&, const unison::sim::FrameInputs& inputs) override
    {
        played = inputs;
        ++runs;
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "InputRecorder";
    }

    [[nodiscard]] const unison::sim::FrameInputs& lastPlayed() const
    {
        return played;
    }

    [[nodiscard]] std::uint32_t runCount() const
    {
        return runs;
    }

private:
    unison::sim::FrameInputs played;
    std::uint32_t runs = 0;
};

unison::session::SessionConfig threePlayers()
{
    unison::session::SessionConfig config;
    config.slotCount = 3;
    config.inputSize = sizeof(SampleInput);
    config.maxPrediction = 4;

    return config;
}

bool confirmAsPlayed(unison::session::Session& session, std::uint32_t frameNumber)
{
    const unison::sim::FrameInputs asPlayed = session.inputs().inputsAt(frameNumber);

    return session.confirm(frameNumber, asPlayed);
}

}

TEST_CASE("a new session starts at the frame it is given and can return to it")
{
    unison::sim::Frame frame;
    frame.frameNumber = 100;
    const unison::sim::SystemPipeline pipeline;

    const unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};

    REQUIRE(session.predictedFrame() == 100U);
    REQUIRE(session.verifiedFrame() == 100U);
    REQUIRE(session.snapshots().holds(100));
}

TEST_CASE("a tick simulates the frame after the predicted one")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};

    session.tick();
    session.tick();

    REQUIRE(session.predictedFrame() == 2U);
    REQUIRE(frame.frameNumber == 2U);
}

TEST_CASE("a tick keeps a snapshot of the frame it simulated")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};

    session.tick();
    session.tick();

    REQUIRE(session.snapshots().holds(1));
    REQUIRE(session.snapshots().holds(2));
}

TEST_CASE("a tick plays the local input and a guess for every other player of the session")
{
    unison::sim::Frame frame;
    InputRecorder recorder;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(recorder);
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    const SampleInput local{1, -2, 300, 5};
    session.setLocalInput(std::as_bytes(std::span{&local, 1}));

    session.tick();

    REQUIRE(recorder.lastPlayed().get<SampleInput>(kLocalSlot).yaw == 300);
    REQUIRE(recorder.lastPlayed().flagsAt(kLocalSlot) == unison::sim::InputFlags::Present);
    REQUIRE(session.inputs().stateAt(1, 0) == unison::session::InputState::Predicted);
    REQUIRE(session.inputs().stateAt(1, 2) == unison::session::InputState::Predicted);
    REQUIRE(session.inputs().stateAt(1, 3) == unison::session::InputState::Missing);
}

TEST_CASE("frames confirmed as they were played are verified without being played again")
{
    unison::sim::Frame frame;
    InputRecorder recorder;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(recorder);
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    session.tick();
    session.tick();
    session.tick();

    const bool confirmedFirst = confirmAsPlayed(session, 1);
    const bool confirmedSecond = confirmAsPlayed(session, 2);
    const bool confirmedThird = confirmAsPlayed(session, 3);

    REQUIRE(confirmedFirst);
    REQUIRE(confirmedSecond);
    REQUIRE(confirmedThird);
    REQUIRE(session.verifiedFrame() == 3U);
    REQUIRE(recorder.runCount() == 3U);
}

TEST_CASE("a frame confirmed before it is played is played on the confirmed inputs")
{
    unison::sim::Frame frame;
    InputRecorder recorder;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(recorder);
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    const SampleInput local{0, 0, 5, 0};
    session.setLocalInput(std::as_bytes(std::span{&local, 1}));
    unison::sim::FrameInputs confirmed;
    confirmed.set(0, SampleInput{0, 0, 7, 0}, unison::sim::InputFlags::Present);
    confirmed.set(kLocalSlot, SampleInput{0, 0, 9, 0}, unison::sim::InputFlags::Dropped);
    confirmed.set(2, SampleInput{0, 0, 11, 0}, unison::sim::InputFlags::Present);
    REQUIRE(session.confirm(1, confirmed));

    session.tick();

    REQUIRE(recorder.lastPlayed().get<SampleInput>(0).yaw == 7);
    REQUIRE(recorder.lastPlayed().get<SampleInput>(kLocalSlot).yaw == 9);
    REQUIRE(recorder.lastPlayed().flagsAt(kLocalSlot) == unison::sim::InputFlags::Dropped);
    REQUIRE(recorder.lastPlayed().get<SampleInput>(2).yaw == 11);
}

TEST_CASE("a frame played on confirmed inputs is verified as soon as it is played")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    REQUIRE(session.confirm(1, unison::sim::FrameInputs{}));

    session.tick();

    REQUIRE(session.verifiedFrame() == 1U);
}

TEST_CASE("the verified frame never runs ahead of the predicted one")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};

    const bool confirmedFirst = session.confirm(1, unison::sim::FrameInputs{});
    const bool confirmedSecond = session.confirm(2, unison::sim::FrameInputs{});

    REQUIRE(confirmedFirst);
    REQUIRE(confirmedSecond);
    REQUIRE(session.verifiedFrame() == 0U);
}

TEST_CASE("verifying frames moves the window on so the session can keep playing")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};

    for (std::uint32_t round = 0; round < 18; ++round)
    {
        session.tick();
        REQUIRE(confirmAsPlayed(session, session.predictedFrame()));
    }

    REQUIRE(probe.failureCount() == 0U);
    REQUIRE(session.verifiedFrame() == 18U);
}

TEST_CASE("a confirmation of a frame already verified is turned away")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    session.tick();
    const unison::sim::FrameInputs firstAsPlayed = session.inputs().inputsAt(1);
    REQUIRE(session.confirm(1, firstAsPlayed));
    session.tick();
    REQUIRE(confirmAsPlayed(session, 2));

    const bool accepted = session.confirm(1, firstAsPlayed);

    REQUIRE_FALSE(accepted);
    REQUIRE(session.verifiedFrame() == 2U);
}

TEST_CASE("a session with more players than a frame has slots breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::SessionConfig config = threePlayers();
    config.slotCount = static_cast<std::uint8_t>(unison::sim::kMaxSlots + 1);

    const unison::session::Session session{frame, pipeline, config, kLocalSlot};

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a session whose local player has no slot in it breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;

    const unison::session::Session session{frame, pipeline, threePlayers(), 3};

    REQUIRE(probe.failureCount() == 1U);
}
