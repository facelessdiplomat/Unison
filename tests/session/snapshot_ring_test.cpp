#include <unison/session/snapshot_ring.hpp>

#include <unison/sim/frame.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstdint>

namespace
{

constexpr std::uint32_t kCapacity = 3;

void storeFrames(unison::session::SnapshotRing& ring,
                 unison::sim::Frame& frame,
                 std::uint32_t firstFrame,
                 std::uint32_t lastFrame)
{
    for (std::uint32_t frameNumber = firstFrame; frameNumber <= lastFrame; ++frameNumber)
    {
        frame.frameNumber = frameNumber;
        ring.store(frame);
    }
}

}

TEST_CASE("a snapshot ring hands back the snapshot stored for a frame")
{
    unison::sim::Frame frame;
    unison::session::SnapshotRing ring{kCapacity};
    frame.frameNumber = 5;
    frame.globals.matchPhase = unison::sim::MatchPhase::Playing;

    ring.store(frame);

    REQUIRE(ring.holds(5));
    REQUIRE(ring.snapshotAt(5).frameNumber == 5U);
    REQUIRE(ring.snapshotAt(5).globals.matchPhase == unison::sim::MatchPhase::Playing);
}

TEST_CASE("a new snapshot ring holds no frame at all")
{
    const unison::session::SnapshotRing ring{kCapacity};

    REQUIRE_FALSE(ring.holds(0));
    REQUIRE_FALSE(ring.holds(1));
}

TEST_CASE("a full snapshot ring overwrites the oldest frame")
{
    unison::sim::Frame frame;
    unison::session::SnapshotRing ring{kCapacity};

    storeFrames(ring, frame, 1, kCapacity + 1);

    REQUIRE_FALSE(ring.holds(1));
    REQUIRE(ring.holds(2));
    REQUIRE(ring.holds(3));
    REQUIRE(ring.holds(4));
    REQUIRE(ring.snapshotAt(4).frameNumber == 4U);
}

TEST_CASE("storing a frame again replaces its snapshot")
{
    unison::sim::Frame frame;
    unison::session::SnapshotRing ring{kCapacity};
    storeFrames(ring, frame, 1, 2);

    frame.globals.matchPhase = unison::sim::MatchPhase::Ended;
    ring.store(frame);

    REQUIRE(ring.snapshotAt(2).globals.matchPhase == unison::sim::MatchPhase::Ended);
    REQUIRE(ring.snapshotAt(1).globals.matchPhase == unison::sim::MatchPhase::Warmup);
}

TEST_CASE("evicting forgets the frames below the verified one")
{
    unison::sim::Frame frame;
    unison::session::SnapshotRing ring{kCapacity};
    storeFrames(ring, frame, 1, 3);

    ring.evictBelow(2);

    REQUIRE_FALSE(ring.holds(1));
    REQUIRE(ring.holds(2));
    REQUIRE(ring.holds(3));
}

TEST_CASE("reading a frame the ring does not hold breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    const unison::session::SnapshotRing ring{kCapacity};

    static_cast<void>(ring.snapshotAt(1));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a snapshot ring needs room for at least one frame")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::session::SnapshotRing ring{0};

    REQUIRE(probe.failureCount() == 1U);
}
