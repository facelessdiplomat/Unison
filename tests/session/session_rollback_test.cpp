#include <unison/session/session.hpp>

#include <support/session_script.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace
{

constexpr std::uint32_t kFrames = 60;
constexpr std::uint32_t kConfirmationDelay = 3;

unison::session::SessionConfig scriptedSession()
{
    unison::session::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = 8;

    return config;
}

unison::sim::FrameInputs firstFrameWithSlotZeroMoving()
{
    unison::sim::FrameInputs confirmed;
    confirmed.set(0, unison::test::inputWithMove(7), unison::sim::InputFlags::Present);
    confirmed.set(unison::test::kSessionLocalSlot, unison::test::inputWithMove(1), unison::sim::InputFlags::Present);
    confirmed.set(2, unison::test::SampleInput{}, unison::sim::InputFlags::None);

    return confirmed;
}

}

TEST_CASE("a session that guessed wrong ends where a match played on the true inputs does")
{
    unison::sim::Frame frame;
    unison::test::addScoredEntity(frame);
    unison::test::InputMixer mixer;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(mixer);
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};

    for (std::uint32_t next = 1; next <= kFrames; ++next)
    {
        const unison::test::SampleInput local =
            unison::test::scriptedSessionInput(next, unison::test::kSessionLocalSlot);
        session.setLocalInput(unison::test::bytesOf(local));
        session.tick();

        if (next > kConfirmationDelay)
        {
            REQUIRE(session.confirm(next - kConfirmationDelay,
                                    unison::test::scriptedSessionInputs(next - kConfirmationDelay)));
        }
    }

    for (std::uint32_t late = kFrames - kConfirmationDelay + 1; late <= kFrames + 1; ++late)
    {
        REQUIRE(session.confirm(late, unison::test::scriptedSessionInputs(late)));
    }

    session.tick();

    REQUIRE(mixer.runCount() > kFrames + 1);
    REQUIRE(session.verifiedFrame() == kFrames + 1);
    REQUIRE(unison::sim::checksumOf(frame) == unison::test::checksumOfScriptedSession(kFrames + 1));
}

TEST_CASE("the verified frame waits for a frame guessed wrong to be played again")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};
    const unison::test::SampleInput local = unison::test::inputWithMove(1);
    session.setLocalInput(unison::test::bytesOf(local));
    session.tick();
    session.tick();

    const bool confirmed = session.confirm(1, firstFrameWithSlotZeroMoving());

    REQUIRE(confirmed);
    REQUIRE(session.verifiedFrame() == 0U);
}

TEST_CASE("a frame guessed wrong is verified once it has been played again")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};
    const unison::test::SampleInput local = unison::test::inputWithMove(1);
    session.setLocalInput(unison::test::bytesOf(local));
    session.tick();
    session.tick();
    REQUIRE(session.confirm(1, firstFrameWithSlotZeroMoving()));

    session.tick();

    REQUIRE(session.verifiedFrame() == 1U);
}

TEST_CASE("a rollback plays the local player's inputs again as they were first played")
{
    unison::sim::Frame frame;
    unison::test::InputRecorder recorder;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(recorder);
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};
    const unison::test::SampleInput first = unison::test::inputWithMove(1);
    const unison::test::SampleInput second = unison::test::inputWithMove(2);
    const unison::test::SampleInput third = unison::test::inputWithMove(3);
    session.setLocalInput(unison::test::bytesOf(first));
    session.tick();
    session.setLocalInput(unison::test::bytesOf(second));
    session.tick();
    session.setLocalInput(unison::test::bytesOf(third));
    REQUIRE(session.confirm(1, firstFrameWithSlotZeroMoving()));

    session.tick();

    REQUIRE(recorder.playedAt(2).get<unison::test::SampleInput>(unison::test::kSessionLocalSlot).moveX == 2);
    REQUIRE(recorder.playedAt(3).get<unison::test::SampleInput>(unison::test::kSessionLocalSlot).moveX == 3);
}

TEST_CASE("a rollback guesses the frames it plays again from the input just confirmed")
{
    unison::sim::Frame frame;
    unison::test::InputRecorder recorder;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(recorder);
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};
    const unison::test::SampleInput local = unison::test::inputWithMove(1);
    session.setLocalInput(unison::test::bytesOf(local));
    session.tick();
    session.tick();
    REQUIRE(session.confirm(1, firstFrameWithSlotZeroMoving()));

    session.tick();

    REQUIRE(recorder.playedAt(2).get<unison::test::SampleInput>(0).moveX == 7);
    REQUIRE(recorder.playedAt(2).flagsAt(0) == unison::sim::InputFlags::Present);
}
