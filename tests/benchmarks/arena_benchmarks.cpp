#include <arena/arena_simulation.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <cstdint>

namespace
{

constexpr std::uint32_t kFramesBeforeMeasuring = 200;
constexpr std::uint32_t kResimulatedFrames = 10;

void play(arena::ArenaSimulation& match, std::uint32_t frames)
{
    for (std::uint32_t played = 0; played < frames; ++played)
    {
        match.advance(unison::test::scriptedInputs(match.frame().frameNumber));
    }
}

}

TEST_CASE("what one frame of the arena costs", "[.benchmark]")
{
    arena::ArenaSimulation match{unison::test::kScriptedPlayers};
    unison::sim::FrameSnapshot snapshot;

    play(match, kFramesBeforeMeasuring);

    BENCHMARK("one tick")
    {
        match.advance(unison::test::scriptedInputs(match.frame().frameNumber));
    };

    unison::sim::takeSnapshot(match.frame(), snapshot);

    BENCHMARK("taking a snapshot")
    {
        unison::sim::takeSnapshot(match.frame(), snapshot);
    };

    BENCHMARK("restoring a snapshot")
    {
        unison::sim::restoreSnapshot(snapshot, match.frame());
    };

    BENCHMARK("checksumming a frame")
    {
        return unison::sim::checksumOf(match.frame());
    };

    BENCHMARK("resimulating ten frames after a rollback")
    {
        unison::sim::restoreSnapshot(snapshot, match.frame());

        play(match, kResimulatedFrames);
    };
}
