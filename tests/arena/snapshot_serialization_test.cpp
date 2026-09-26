#include <unison/session/snapshot_serializer.hpp>

#include <arena/arena_simulation.hpp>

#include <support/arena_script.hpp>
#include <unison/core/error.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kFramesBefore = 200;
constexpr std::uint32_t kFramesAfter = 100;

void play(arena::ArenaSimulation& match, std::uint32_t frames)
{
    for (std::uint32_t played = 0; played < frames; ++played)
    {
        match.advance(unison::test::scriptedInputs(match.frame().frameNumber));
    }
}

std::vector<std::uint64_t> checksumsOver(arena::ArenaSimulation& match, std::uint32_t frames)
{
    std::vector<std::uint64_t> checksums;

    for (std::uint32_t played = 0; played < frames; ++played)
    {
        play(match, 1);
        checksums.push_back(unison::sim::checksumOf(match.frame()));
    }

    return checksums;
}

}

TEST_CASE("an arena frame serialised and restored into another match plays on exactly as the original")
{
    arena::ArenaSimulation original{unison::test::kScriptedPlayers};
    play(original, kFramesBefore);
    unison::sim::FrameSnapshot taken;
    unison::sim::takeSnapshot(original.frame(), taken);
    std::vector<std::byte> bytes;
    unison::session::serializeSnapshot(taken, bytes);
    unison::sim::FrameSnapshot read;
    REQUIRE(unison::session::deserializeSnapshot(bytes, read).has_value());
    arena::ArenaSimulation restored{unison::test::kScriptedPlayers};

    unison::sim::restoreSnapshot(read, restored.frame());

    REQUIRE(unison::sim::checksumOf(restored.frame()) == unison::sim::checksumOf(original.frame()));
    REQUIRE(checksumsOver(restored, kFramesAfter) == checksumsOver(original, kFramesAfter));
}
