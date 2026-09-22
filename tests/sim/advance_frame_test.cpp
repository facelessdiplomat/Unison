#include <unison/sim/advance_frame.hpp>

#include <unison/sim/entity_lifecycle.hpp>

#include <catch2/catch_test_macros.hpp>

#include <xmmintrin.h>

#include <cstdint>
#include <string_view>

namespace
{

constexpr std::uint32_t kFlushToZero = 0x8000U;

class ControlWordProbe final : public unison::sim::ISystem
{
public:
    void update(unison::sim::Frame&, const unison::sim::FrameInputs&) override
    {
        observed = _mm_getcsr();
        ++runs;
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "ControlWordProbe";
    }

    [[nodiscard]] std::uint32_t controlWord() const
    {
        return observed;
    }

    [[nodiscard]] std::uint32_t runCount() const
    {
        return runs;
    }

private:
    std::uint32_t observed = 0;
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
    const std::uint32_t hostControlWord = _mm_getcsr();
    _mm_setcsr(hostControlWord | kFlushToZero);

    ControlWordProbe probe;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(probe);

    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    unison::sim::advanceFrame(frame, pipeline, inputs);

    _mm_setcsr(hostControlWord);

    REQUIRE((probe.controlWord() & kFlushToZero) == 0U);
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
    const std::uint32_t hostControlWord = _mm_getcsr();
    _mm_setcsr(hostControlWord | kFlushToZero);
    const std::uint32_t withFlushToZero = _mm_getcsr();

    const unison::sim::SystemPipeline pipeline;
    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    unison::sim::advanceFrame(frame, pipeline, inputs);

    const std::uint32_t afterTick = _mm_getcsr();
    _mm_setcsr(hostControlWord);

    REQUIRE(afterTick == withFlushToZero);
}
