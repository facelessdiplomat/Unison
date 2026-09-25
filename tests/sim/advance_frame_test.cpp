#include <unison/sim/advance_frame.hpp>

#include <unison/core/fp_control_word.hpp>
#include <unison/sim/entity_lifecycle.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>

namespace
{

class ControlWordProbe final : public unison::sim::ISystem
{
public:
    void update(unison::sim::Frame&, const unison::sim::FrameInputs&) override
    {
        observed = unison::readFpControlWord();
        ++runs;
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "ControlWordProbe";
    }

    [[nodiscard]] unison::FpControlWord controlWord() const
    {
        return observed;
    }

    [[nodiscard]] std::uint32_t runCount() const
    {
        return runs;
    }

private:
    unison::FpControlWord observed = 0;
    std::uint32_t runs = 0;
};

class SpawningSystem final : public unison::sim::ISystem
{
public:
    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs&) override
    {
        static_cast<void>(unison::sim::createEntity(frame));
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "SpawningSystem";
    }
};

}

TEST_CASE("the deterministic control word is in force while systems run")
{
    const unison::FpControlWord hostControlWord = unison::readFpControlWord();
    unison::writeFpControlWord(hostControlWord | unison::kFlushToZeroBits);

    ControlWordProbe probe;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(probe);

    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    unison::sim::advanceFrame(frame, pipeline, inputs);

    unison::writeFpControlWord(hostControlWord);

    REQUIRE((probe.controlWord() & unison::kFlushToZeroBits) == 0U);
}

TEST_CASE("advancing runs the pipeline once and moves the frame on")
{
    ControlWordProbe probe;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(probe);

    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    unison::sim::advanceFrame(frame, pipeline, inputs);

    REQUIRE(probe.runCount() == 1U);
    REQUIRE(frame.frameNumber == 1U);
}

TEST_CASE("an event is keyed to the tick that raised it")
{
    SpawningSystem spawning;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(spawning);

    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    unison::sim::advanceFrame(frame, pipeline, inputs);

    REQUIRE(frame.events.size() == 1U);
    REQUIRE(frame.events.keyAt(0).frame == 0U);
    REQUIRE(frame.frameNumber == 1U);
}

TEST_CASE("the events of a tick do not outlive it")
{
    SpawningSystem spawning;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(spawning);

    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    unison::sim::advanceFrame(frame, pipeline, inputs);
    unison::sim::advanceFrame(frame, pipeline, inputs);

    REQUIRE(frame.events.size() == 1U);
    REQUIRE(frame.events.keyAt(0).frame == 1U);
}

TEST_CASE("the host keeps its own control word after a tick")
{
    const unison::FpControlWord hostControlWord = unison::readFpControlWord();
    unison::writeFpControlWord(hostControlWord | unison::kFlushToZeroBits);
    const unison::FpControlWord withFlushToZero = unison::readFpControlWord();

    const unison::sim::SystemPipeline pipeline;
    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    unison::sim::advanceFrame(frame, pipeline, inputs);

    const unison::FpControlWord afterTick = unison::readFpControlWord();
    unison::writeFpControlWord(hostControlWord);

    REQUIRE(afterTick == withFlushToZero);
}
