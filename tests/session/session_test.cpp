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
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "InputRecorder";
    }

    [[nodiscard]] const unison::sim::FrameInputs& lastPlayed() const
    {
        return played;
    }

private:
    unison::sim::FrameInputs played;
};

unison::session::SessionConfig threePlayers()
{
    unison::session::SessionConfig config;
    config.slotCount = 3;
    config.inputSize = sizeof(SampleInput);
    config.maxPrediction = 4;

    return config;
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
