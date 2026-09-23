#include <unison/session/input_timeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>
#include <support/session_script.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

using unison::session::InputState;
using unison::test::SampleInput;

constexpr std::size_t kSlots = unison::test::kSessionSlots;
constexpr std::size_t kLocalSlot = unison::test::kSessionLocalSlot;
constexpr std::size_t kRemoteSlot = 2;
constexpr std::uint32_t kCapacity = 8;

unison::sim::FrameInputs everySlotMoving(std::int8_t moveX)
{
    unison::sim::FrameInputs inputs;

    for (std::size_t slot = 0; slot < kSlots; ++slot)
    {
        inputs.set(slot, unison::test::inputWithMove(moveX), unison::sim::InputFlags::Present);
    }

    return inputs;
}

std::int8_t moveOf(const unison::session::InputTimeline& timeline, std::uint32_t frame, std::size_t slot)
{
    return timeline.inputsAt(frame).get<SampleInput>(slot).moveX;
}

unison::session::InputTimeline playedOnGuesses(std::int8_t confirmedMove, std::int8_t localMove)
{
    unison::session::InputTimeline timeline{kSlots, kLocalSlot, kCapacity};
    timeline.confirm(1, everySlotMoving(confirmedMove));
    timeline.setLocalInput(unison::test::bytesOf(unison::test::inputWithMove(localMove)));
    timeline.sampleLocal(2);
    timeline.guessUnconfirmed(2);

    return timeline;
}

}

TEST_CASE("sampling writes the local player's input into its slot, still waiting for the relay")
{
    unison::session::InputTimeline timeline{kSlots, kLocalSlot, kCapacity};
    timeline.setLocalInput(unison::test::bytesOf(unison::test::inputWithMove(4)));

    timeline.sampleLocal(1);

    REQUIRE(moveOf(timeline, 1, kLocalSlot) == 4);
    REQUIRE(timeline.buffer().stateAt(1, kLocalSlot) == InputState::Predicted);
}

TEST_CASE("sampling leaves a local input the relay confirmed as it is")
{
    unison::session::InputTimeline timeline{kSlots, kLocalSlot, kCapacity};
    timeline.confirm(1, everySlotMoving(2));
    timeline.setLocalInput(unison::test::bytesOf(unison::test::inputWithMove(4)));

    timeline.sampleLocal(1);

    REQUIRE(moveOf(timeline, 1, kLocalSlot) == 2);
    REQUIRE(timeline.buffer().stateAt(1, kLocalSlot) == InputState::Confirmed);
}

TEST_CASE("a guess fills every slot of a frame but the local one")
{
    const unison::session::InputTimeline timeline = playedOnGuesses(2, 4);

    REQUIRE(moveOf(timeline, 2, 0) == 2);
    REQUIRE(moveOf(timeline, 2, kRemoteSlot) == 2);
    REQUIRE(moveOf(timeline, 2, kLocalSlot) == 4);
    REQUIRE(timeline.buffer().stateAt(2, 0) == InputState::Predicted);
    REQUIRE(timeline.buffer().stateAt(2, kRemoteSlot) == InputState::Predicted);
}

TEST_CASE("a guess leaves a frame the relay confirmed as it is")
{
    unison::session::InputTimeline timeline{kSlots, kLocalSlot, kCapacity};
    timeline.confirm(1, everySlotMoving(1));
    timeline.confirm(2, everySlotMoving(3));

    timeline.guessUnconfirmed(2);

    REQUIRE(moveOf(timeline, 2, 0) == 3);
    REQUIRE(moveOf(timeline, 2, kRemoteSlot) == 3);
    REQUIRE(timeline.buffer().stateAt(2, 0) == InputState::Confirmed);
}

TEST_CASE("a confirmation settles every slot of the session and no other")
{
    unison::session::InputTimeline timeline{kSlots, kLocalSlot, kCapacity};

    timeline.confirm(1, everySlotMoving(3));

    for (std::size_t slot = 0; slot < kSlots; ++slot)
    {
        REQUIRE(moveOf(timeline, 1, slot) == 3);
        REQUIRE(timeline.buffer().stateAt(1, slot) == InputState::Confirmed);
    }

    REQUIRE(timeline.buffer().stateAt(1, kSlots) == InputState::Missing);
}

TEST_CASE("a frame counts as confirmed once the relay has settled it")
{
    unison::session::InputTimeline timeline = playedOnGuesses(2, 4);
    const bool confirmedWhilePlayedOnGuesses = timeline.isConfirmed(2);

    timeline.confirm(2, everySlotMoving(2));

    REQUIRE_FALSE(confirmedWhilePlayedOnGuesses);
    REQUIRE(timeline.isConfirmed(2));
}

TEST_CASE("a frame that holds what the relay settled matches its confirmation")
{
    const unison::session::InputTimeline timeline = playedOnGuesses(2, 2);

    REQUIRE(timeline.matches(2, everySlotMoving(2)));
}

TEST_CASE("a frame whose input differs in one slot does not match its confirmation")
{
    const unison::session::InputTimeline timeline = playedOnGuesses(2, 2);
    unison::sim::FrameInputs confirmed = everySlotMoving(2);
    confirmed.set(kRemoteSlot, unison::test::inputWithMove(1), unison::sim::InputFlags::Present);

    REQUIRE_FALSE(timeline.matches(2, confirmed));
}

TEST_CASE("a frame whose flags differ in one slot does not match its confirmation")
{
    const unison::session::InputTimeline timeline = playedOnGuesses(2, 2);
    unison::sim::FrameInputs confirmed = everySlotMoving(2);
    confirmed.setFlags(kRemoteSlot, unison::sim::InputFlags::Dropped);

    REQUIRE_FALSE(timeline.matches(2, confirmed));
}

TEST_CASE("confirming a frame outside the window breaks a contract")
{
    unison::session::InputTimeline timeline{kSlots, kLocalSlot, kCapacity};
    const unison::test::FatalHandlerProbe probe;

    timeline.confirm(kCapacity, everySlotMoving(1));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a timeline whose local player has no slot in the session breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::session::InputTimeline timeline{kSlots, kSlots, kCapacity};

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a timeline of more slots than a frame holds breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::session::InputTimeline timeline{unison::sim::kMaxSlots + 1, kLocalSlot, kCapacity};

    REQUIRE(probe.failureCount() == 1U);
}
