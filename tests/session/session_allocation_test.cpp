#include <unison/session/session.hpp>

#include <support/allocation_probe.hpp>
#include <support/session_script.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace
{

constexpr std::uint32_t kWarmUpTicks = 240;
constexpr std::uint32_t kMeasuredTicks = 240;
constexpr std::uint32_t kConfirmationDelay = 3;
constexpr std::uint32_t kTicksBetweenRemoteChanges = 4;

unison::net::SessionConfig scriptedSession()
{
    unison::net::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = 8;
    config.checksumInterval = 1;

    return config;
}

bool playTickLikeAHost(unison::session::Session& session, std::uint32_t next)
{
    session.setLocalInput(
        unison::test::bytesOf(unison::test::scriptedSessionInput(next, unison::test::kSessionLocalSlot)));
    session.tick();

    const bool isConfirmed =
        next <= kConfirmationDelay ||
        session.confirm(next - kConfirmationDelay, unison::test::scriptedSessionInputs(next - kConfirmationDelay));

    session.clearVerifiedChecksums();
    session.clearEventChanges();

    return isConfirmed;
}

}

TEST_CASE("a session that keeps rolling back takes nothing from the C++ heap once warmed up")
{
    unison::sim::Frame frame;
    unison::test::addScoredEntity(frame);
    unison::test::InputMixer mixer;
    unison::test::MoveAnnouncer moves;
    unison::test::SettleAnnouncer settles;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(mixer);
    pipeline.add(moves);
    pipeline.add(settles);
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};

    for (std::uint32_t next = 1; next <= kWarmUpTicks; ++next)
    {
        REQUIRE(playTickLikeAHost(session, next));
    }

    const std::uint32_t rollbacksBefore = session.rollbackStats().rollbacks;
    std::uint32_t confirmations = 0;
    const unison::test::AllocationProbe probe;

    for (std::uint32_t next = kWarmUpTicks + 1; next <= kWarmUpTicks + kMeasuredTicks; ++next)
    {
        confirmations += playTickLikeAHost(session, next) ? 1U : 0U;
    }

    REQUIRE(probe.cppAllocations() == 0U);
    REQUIRE(confirmations == kMeasuredTicks);
    REQUIRE(session.rollbackStats().rollbacks - rollbacksBefore >= kMeasuredTicks / kTicksBetweenRemoteChanges);
}
