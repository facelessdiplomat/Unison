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

unison::net::SessionConfig scriptedSession()
{
    unison::net::SessionConfig config;
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

TEST_CASE("rollback statistics count every rollback, the deepest one and every frame played again")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};
    const unison::test::SampleInput local = unison::test::inputWithMove(1);
    session.setLocalInput(unison::test::bytesOf(local));
    session.tick();
    session.tick();
    session.tick();
    REQUIRE(session.confirm(1, firstFrameWithSlotZeroMoving()));
    session.tick();
    unison::sim::FrameInputs thirdContradicted = session.inputs().inputsAt(3);
    thirdContradicted.set(2, unison::test::inputWithMove(-1), unison::sim::InputFlags::Present);
    REQUIRE(session.confirm(3, thirdContradicted));

    session.tick();

    const unison::session::RollbackStats& stats = session.rollbackStats();
    REQUIRE(stats.rollbacks == 2U);
    REQUIRE(stats.deepestRollback == 3U);
    REQUIRE(stats.resimulatedFrames == 5U);
    REQUIRE(stats.framesPlayed == 5U);
}

TEST_CASE("rollback statistics give their rates per second of play")
{
    unison::session::RollbackStats stats;
    stats.rollbacks = 2;
    stats.resimulatedFrames = 5;
    stats.framesPlayed = 5;

    REQUIRE(stats.rollbacksPerSecond(60) == 24.0);
    REQUIRE(stats.resimulatedFramesPerSecond(60) == 60.0);
}

TEST_CASE("rollback statistics have no rates before a frame is played")
{
    const unison::session::RollbackStats stats;

    REQUIRE(stats.rollbacksPerSecond(60) == 0.0);
    REQUIRE(stats.resimulatedFramesPerSecond(60) == 0.0);
}

TEST_CASE("rollback statistics give the frames a rollback plays again on average")
{
    unison::session::RollbackStats stats;
    stats.rollbacks = 2;
    stats.resimulatedFrames = 5;

    REQUIRE(stats.meanRollbackDepth() == 2.5);
}

TEST_CASE("rollback statistics have no mean depth before the first rollback")
{
    const unison::session::RollbackStats stats;

    REQUIRE(stats.meanRollbackDepth() == 0.0);
}

TEST_CASE("rollback statistics count the ticks that had to wait for the relay")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::net::SessionConfig narrow = scriptedSession();
    narrow.maxPrediction = 2;
    unison::session::Session session{frame, pipeline, narrow, unison::test::kSessionLocalSlot};

    for (std::uint32_t round = 0; round < 5; ++round)
    {
        session.tick();
    }

    REQUIRE(session.rollbackStats().stalledTicks == 3U);
    REQUIRE(session.rollbackStats().framesPlayed == 2U);
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
