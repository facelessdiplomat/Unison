#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <unison/sim/frame_checksum.hpp>

#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kFrames = 600;

std::vector<std::uint64_t> checksumsOfAMatch(std::uint32_t frames)
{
    arena::ArenaSimulation match{unison::test::kScriptedPlayers};

    std::vector<std::uint64_t> checksums;
    checksums.reserve(frames);

    for (std::uint32_t frameNumber = 0; frameNumber < frames; ++frameNumber)
    {
        match.advance(unison::test::scriptedInputs(frameNumber));
        checksums.push_back(unison::sim::checksumOf(match.frame()));
    }

    return checksums;
}

}

TEST_CASE("two matches played side by side agree on every frame")
{
    arena::ArenaSimulation one{unison::test::kScriptedPlayers};
    arena::ArenaSimulation other{unison::test::kScriptedPlayers};

    for (std::uint32_t frameNumber = 0; frameNumber < kFrames; ++frameNumber)
    {
        one.advance(unison::test::scriptedInputs(frameNumber));
        other.advance(unison::test::scriptedInputs(frameNumber));

        if (unison::sim::checksumOf(one.frame()) != unison::sim::checksumOf(other.frame()))
        {
            FAIL("the matches parted ways on frame " << frameNumber);
        }
    }

    REQUIRE(unison::sim::checksumOf(one.frame()) == unison::sim::checksumOf(other.frame()));
}

TEST_CASE("a match played after another is played the same way")
{
    const std::vector<std::uint64_t> first = checksumsOfAMatch(kFrames);
    const std::vector<std::uint64_t> second = checksumsOfAMatch(kFrames);

    REQUIRE(first.size() == kFrames);

    for (std::uint32_t frameNumber = 0; frameNumber < kFrames; ++frameNumber)
    {
        if (first[frameNumber] != second[frameNumber])
        {
            FAIL("the second match parted ways with the first on frame " << frameNumber);
        }
    }

    REQUIRE(first == second);
}

TEST_CASE("a match does not stand still")
{
    const std::vector<std::uint64_t> checksums = checksumsOfAMatch(120U);

    REQUIRE(checksums.front() != checksums.back());
}
