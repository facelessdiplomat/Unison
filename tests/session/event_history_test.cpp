#include <unison/session/event_history.hpp>

#include <support/fatal_handler_probe.hpp>
#include <support/session_script.hpp>
#include <unison/sim/event_buffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kCapacity = 4;

}

TEST_CASE("a frame played for the first time raises every predicted event it raised")
{
    unison::session::EventHistory history{kCapacity};
    unison::sim::EventBuffer events;
    const unison::sim::EventKey first = events.raise(4, unison::test::SlotMoved{0});
    const unison::sim::EventKey second = events.raise(4, unison::test::SlotMoved{2});
    unison::session::EventChanges changes;

    history.record(5, events, changes);

    REQUIRE(changes.raised == std::vector<unison::sim::EventKey>{first, second});
    REQUIRE(changes.cancelled.empty());
}

TEST_CASE("a frame played again raises only the predicted events it did not raise before")
{
    unison::session::EventHistory history{kCapacity};
    unison::sim::EventBuffer before;
    before.raise(4, unison::test::SlotMoved{0});
    unison::session::EventChanges firstPlay;
    history.record(5, before, firstPlay);
    unison::sim::EventBuffer after;
    after.raise(4, unison::test::SlotMoved{0});
    const unison::sim::EventKey added = after.raise(4, unison::test::SlotMoved{2});
    unison::session::EventChanges changes;

    history.record(5, after, changes);

    REQUIRE(changes.raised == std::vector<unison::sim::EventKey>{added});
    REQUIRE(changes.cancelled.empty());
}

TEST_CASE("a frame played again cancels the predicted events it no longer raises")
{
    unison::session::EventHistory history{kCapacity};
    unison::sim::EventBuffer before;
    before.raise(4, unison::test::SlotMoved{0});
    const unison::sim::EventKey dropped = before.raise(4, unison::test::SlotMoved{2});
    unison::session::EventChanges firstPlay;
    history.record(5, before, firstPlay);
    unison::sim::EventBuffer after;
    after.raise(4, unison::test::SlotMoved{0});
    unison::session::EventChanges changes;

    history.record(5, after, changes);

    REQUIRE(changes.raised.empty());
    REQUIRE(changes.cancelled == std::vector<unison::sim::EventKey>{dropped});
}

TEST_CASE("a replay neither raises nor cancels events that wait for their frame to be verified")
{
    unison::session::EventHistory history{kCapacity};
    unison::sim::EventBuffer before;
    before.raise(4, unison::test::SlotSettled{0});
    unison::session::EventChanges firstPlay;
    history.record(5, before, firstPlay);
    const unison::sim::EventBuffer after;
    unison::session::EventChanges changes;

    history.record(5, after, changes);

    REQUIRE(firstPlay.raised.empty());
    REQUIRE(changes.raised.empty());
    REQUIRE(changes.cancelled.empty());
}

TEST_CASE("a frame taking the place of an older one in the history is played for the first time")
{
    unison::session::EventHistory history{kCapacity};
    unison::sim::EventBuffer older;
    older.raise(4, unison::test::SlotMoved{0});
    unison::session::EventChanges firstPlay;
    history.record(5, older, firstPlay);
    unison::sim::EventBuffer newer;
    const unison::sim::EventKey raised = newer.raise(8, unison::test::SlotMoved{0});
    unison::session::EventChanges changes;

    history.record(5 + kCapacity, newer, changes);

    REQUIRE(changes.raised == std::vector<unison::sim::EventKey>{raised});
    REQUIRE(changes.cancelled.empty());
}

TEST_CASE("an event history needs room for at least one frame")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::session::EventHistory history{0};

    REQUIRE(probe.failureCount() == 1U);
}
