#include <unison/session/session.hpp>

#include <support/session_script.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace
{

constexpr std::uint32_t kFrames = 60;
constexpr std::uint32_t kRelayAnswerTicks = 2;

unison::net::SessionConfig scriptedSession()
{
    unison::net::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = 8;

    return config;
}

unison::test::SampleInput localInputOf(std::uint32_t frameNumber, std::uint32_t inputDelay)
{
    if (frameNumber <= inputDelay)
    {
        return unison::test::SampleInput{};
    }

    return unison::test::scriptedSessionInput(frameNumber, unison::test::kSessionLocalSlot);
}

unison::sim::FrameInputs settledInputsOf(std::uint32_t frameNumber, std::uint32_t inputDelay)
{
    unison::sim::FrameInputs settled = unison::test::scriptedSessionInputs(frameNumber);
    settled.set(
        unison::test::kSessionLocalSlot, localInputOf(frameNumber, inputDelay), unison::sim::InputFlags::Present);

    return settled;
}

std::uint32_t rollbacksWithInputDelay(std::uint32_t inputDelay)
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot, inputDelay};

    for (std::uint32_t tick = 1; tick <= kFrames; ++tick)
    {
        if (tick + inputDelay > kRelayAnswerTicks)
        {
            const std::uint32_t settledFrame = tick + inputDelay - kRelayAnswerTicks;
            REQUIRE(session.confirm(settledFrame, settledInputsOf(settledFrame, inputDelay)));
        }

        const unison::test::SampleInput local = localInputOf(tick + inputDelay, inputDelay);
        session.setLocalInput(unison::test::bytesOf(local));
        session.tick();
    }

    return session.rollbackStats().rollbacks;
}

}

TEST_CASE("an input delay plays the local input that many frames later")
{
    unison::sim::Frame frame;
    unison::test::InputRecorder recorder;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(recorder);
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot, 2};
    const unison::test::SampleInput local = unison::test::inputWithMove(4);
    session.setLocalInput(unison::test::bytesOf(local));

    session.tick();
    session.tick();
    session.tick();

    REQUIRE(recorder.playedAt(1).get<unison::test::SampleInput>(unison::test::kSessionLocalSlot).moveX == 0);
    REQUIRE(recorder.playedAt(2).get<unison::test::SampleInput>(unison::test::kSessionLocalSlot).moveX == 0);
    REQUIRE(recorder.playedAt(3).get<unison::test::SampleInput>(unison::test::kSessionLocalSlot).moveX == 4);
}

TEST_CASE("an input delay as long as the relay takes to answer spares every rollback")
{
    const std::uint32_t withoutDelay = rollbacksWithInputDelay(0);
    const std::uint32_t withDelay = rollbacksWithInputDelay(kRelayAnswerTicks);

    REQUIRE(withoutDelay > 0U);
    REQUIRE(withDelay == 0U);
}
