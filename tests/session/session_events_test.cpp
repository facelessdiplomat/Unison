#include <unison/session/session.hpp>

#include <support/session_script.hpp>
#include <unison/sim/event_buffer.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kSlotMoved = unison::sim::EventTraits<unison::test::SlotMoved>::typeId;

unison::session::SessionConfig scriptedSession()
{
    unison::session::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = 8;

    return config;
}

unison::sim::FrameInputs settledWithMoves(std::int8_t firstSlotMove, std::int8_t lastSlotMove)
{
    unison::sim::FrameInputs settled;
    settled.set(0, unison::test::inputWithMove(firstSlotMove), unison::sim::InputFlags::Present);
    settled.set(unison::test::kSessionLocalSlot, unison::test::SampleInput{}, unison::sim::InputFlags::Present);
    settled.set(2, unison::test::inputWithMove(lastSlotMove), unison::sim::InputFlags::Present);

    return settled;
}

}

TEST_CASE("a predicted event that a replay no longer raises is cancelled")
{
    unison::sim::Frame frame;
    unison::test::MoveAnnouncer announcer;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(announcer);
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};
    REQUIRE(session.confirm(1, settledWithMoves(1, 0)));
    session.tick();
    session.tick();
    session.tick();
    session.clearEventChanges();
    REQUIRE(session.confirm(2, settledWithMoves(0, 0)));

    session.tick();

    const std::vector<unison::sim::EventKey> cancelled{unison::sim::EventKey{1, kSlotMoved, 0},
                                                       unison::sim::EventKey{2, kSlotMoved, 0}};
    REQUIRE(session.eventChanges().cancelled == cancelled);
    REQUIRE(session.eventChanges().raised.empty());
}

TEST_CASE("a predicted event that only a replay raises is raised then")
{
    unison::sim::Frame frame;
    unison::test::MoveAnnouncer announcer;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(announcer);
    unison::session::Session session{frame, pipeline, scriptedSession(), unison::test::kSessionLocalSlot};
    session.tick();
    session.tick();
    session.clearEventChanges();
    REQUIRE(session.confirm(1, settledWithMoves(0, 5)));

    session.tick();

    const std::vector<unison::sim::EventKey> raised{unison::sim::EventKey{0, kSlotMoved, 0},
                                                    unison::sim::EventKey{1, kSlotMoved, 0},
                                                    unison::sim::EventKey{2, kSlotMoved, 0}};
    REQUIRE(session.eventChanges().raised == raised);
    REQUIRE(session.eventChanges().cancelled.empty());
}
