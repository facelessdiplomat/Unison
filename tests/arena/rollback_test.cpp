#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <unison/core/rng.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kFrames = 600;
constexpr std::uint32_t kRollbacks = 20;
constexpr std::uint32_t kReplayedFrames = 15;
constexpr std::uint64_t kSeed = 20260922;

std::vector<std::uint32_t> framesToRollBackTo()
{
    constexpr std::uint32_t kFirstFrame = 20;
    constexpr std::uint32_t kStride = (kFrames - kFirstFrame - kReplayedFrames) / kRollbacks;
    constexpr std::int32_t kDrift = static_cast<std::int32_t>(kStride - kReplayedFrames) - 1;

    static_assert(kDrift > 0, "a rollback has to finish before the next one starts");

    unison::Rng rng{kSeed};
    std::vector<std::uint32_t> frames;

    for (std::uint32_t index = 0; index < kRollbacks; ++index)
    {
        frames.push_back(kFirstFrame + index * kStride + static_cast<std::uint32_t>(rng.nextInRange(0, kDrift)));
    }

    return frames;
}

void play(arena::ArenaSimulation& match, std::uint32_t frames)
{
    for (std::uint32_t played = 0; played < frames; ++played)
    {
        match.advance(unison::test::scriptedInputs(match.frame().frameNumber));
    }
}

}

TEST_CASE("the arena plays on the same way from any snapshot it is taken back to")
{
    const std::vector<std::uint32_t> rollbacks = framesToRollBackTo();

    REQUIRE(rollbacks.size() == kRollbacks);

    arena::ArenaSimulation match{unison::test::kScriptedPlayers};
    unison::sim::FrameSnapshot snapshot;

    std::size_t next = 0;

    while (match.frame().frameNumber < kFrames)
    {
        if (next >= rollbacks.size() || match.frame().frameNumber != rollbacks[next])
        {
            play(match, 1U);

            continue;
        }

        const std::uint32_t takenAt = match.frame().frameNumber;

        unison::sim::takeSnapshot(match.frame(), snapshot);

        play(match, kReplayedFrames);

        const std::uint64_t playedOn = unison::sim::checksumOf(match.frame());

        unison::sim::restoreSnapshot(snapshot, match.frame());

        REQUIRE(match.frame().frameNumber == takenAt);

        play(match, kReplayedFrames);

        if (unison::sim::checksumOf(match.frame()) != playedOn)
        {
            FAIL("the match played on differently after being taken back to frame " << takenAt);
        }

        ++next;
    }

    REQUIRE(next == kRollbacks);
}

TEST_CASE("a snapshot of the arena brings back the frame it was taken at")
{
    arena::ArenaSimulation match{unison::test::kScriptedPlayers};

    play(match, 120U);

    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(match.frame(), snapshot);

    const std::uint64_t taken = unison::sim::checksumOf(match.frame());

    play(match, 60U);

    REQUIRE(unison::sim::checksumOf(match.frame()) != taken);

    unison::sim::restoreSnapshot(snapshot, match.frame());

    REQUIRE(unison::sim::checksumOf(match.frame()) == taken);
}
