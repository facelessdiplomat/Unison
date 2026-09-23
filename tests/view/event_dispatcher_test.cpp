#include <unison/view/event_dispatcher.hpp>

#include <support/session_script.hpp>
#include <unison/session/event_history.hpp>
#include <unison/sim/event_buffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kFrame = 4;

unison::session::EventChanges raisingMoveOf(std::uint32_t slot)
{
    unison::session::EventChanges changes;
    changes.raised.raise(kFrame, unison::test::SlotMoved{slot});

    return changes;
}

unison::session::EventChanges cancelling(const unison::sim::EventKey& key)
{
    unison::session::EventChanges changes;
    changes.cancelled.push_back(key);

    return changes;
}

}

TEST_CASE("a raised event reaches the handler registered for its type, payload and all")
{
    unison::view::EventDispatcher dispatcher;
    std::vector<std::uint32_t> movedSlots;
    dispatcher.on<unison::test::SlotMoved>(
        [&movedSlots](const unison::sim::EventKey&, const unison::test::SlotMoved& moved)
        { movedSlots.push_back(moved.slot); });

    dispatcher.dispatch(raisingMoveOf(2));

    REQUIRE(movedSlots == std::vector<std::uint32_t>{2});
}

TEST_CASE("a handler runs once per key even if the same changes are handed over twice")
{
    unison::view::EventDispatcher dispatcher;
    std::uint32_t shown = 0;
    dispatcher.on<unison::test::SlotMoved>([&shown](const unison::sim::EventKey&, const unison::test::SlotMoved&)
                                           { ++shown; });
    const unison::session::EventChanges changes = raisingMoveOf(2);

    dispatcher.dispatch(changes);
    dispatcher.dispatch(changes);

    REQUIRE(shown == 1U);
}

TEST_CASE("a cancellation reaches its handler for an event that was shown")
{
    unison::view::EventDispatcher dispatcher;
    std::vector<unison::sim::EventKey> takenBack;
    dispatcher.onCancelled<unison::test::SlotMoved>([&takenBack](const unison::sim::EventKey& key)
                                                    { takenBack.push_back(key); });
    const unison::session::EventChanges raised = raisingMoveOf(2);
    dispatcher.dispatch(raised);

    dispatcher.dispatch(cancelling(raised.raised.keyAt(0)));

    REQUIRE(takenBack == std::vector<unison::sim::EventKey>{raised.raised.keyAt(0)});
}

TEST_CASE("a cancellation of an event never shown reaches no handler")
{
    unison::view::EventDispatcher dispatcher;
    std::uint32_t takenBack = 0;
    dispatcher.onCancelled<unison::test::SlotMoved>([&takenBack](const unison::sim::EventKey&) { ++takenBack; });
    const unison::sim::EventKey neverShown{kFrame, unison::sim::EventTraits<unison::test::SlotMoved>::typeId, 0};

    dispatcher.dispatch(cancelling(neverShown));

    REQUIRE(takenBack == 0U);
}

TEST_CASE("an event cancelled and then raised again is shown again")
{
    unison::view::EventDispatcher dispatcher;
    std::uint32_t shown = 0;
    dispatcher.on<unison::test::SlotMoved>([&shown](const unison::sim::EventKey&, const unison::test::SlotMoved&)
                                           { ++shown; });
    const unison::session::EventChanges raised = raisingMoveOf(2);
    dispatcher.dispatch(raised);
    dispatcher.dispatch(cancelling(raised.raised.keyAt(0)));

    dispatcher.dispatch(raised);

    REQUIRE(shown == 2U);
}

TEST_CASE("events of a type nobody handles are passed over")
{
    unison::view::EventDispatcher dispatcher;
    std::uint32_t shown = 0;
    dispatcher.on<unison::test::SlotMoved>([&shown](const unison::sim::EventKey&, const unison::test::SlotMoved&)
                                           { ++shown; });
    unison::session::EventChanges changes;
    changes.raised.raise(kFrame, unison::test::SlotSettled{1});

    dispatcher.dispatch(changes);

    REQUIRE(shown == 0U);
}

TEST_CASE("an event of a frame the dispatcher was told to forget is shown again when handed over again")
{
    unison::view::EventDispatcher dispatcher;
    std::uint32_t shown = 0;
    dispatcher.on<unison::test::SlotMoved>([&shown](const unison::sim::EventKey&, const unison::test::SlotMoved&)
                                           { ++shown; });
    const unison::session::EventChanges raised = raisingMoveOf(2);
    dispatcher.dispatch(raised);

    dispatcher.forgetBelow(kFrame + 1);
    dispatcher.dispatch(raised);

    REQUIRE(shown == 2U);
}

TEST_CASE("forgetting the frames below one keeps the events of that frame and after")
{
    unison::view::EventDispatcher dispatcher;
    std::uint32_t shown = 0;
    dispatcher.on<unison::test::SlotMoved>([&shown](const unison::sim::EventKey&, const unison::test::SlotMoved&)
                                           { ++shown; });
    const unison::session::EventChanges raised = raisingMoveOf(2);
    dispatcher.dispatch(raised);

    dispatcher.forgetBelow(kFrame);
    dispatcher.dispatch(raised);

    REQUIRE(shown == 1U);
}
