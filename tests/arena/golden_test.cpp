#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <unison/sim/frame_checksum.hpp>

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

namespace
{

constexpr std::uint32_t kFrames = 600;
constexpr std::uint32_t kEveryNthFrame = 10;

struct RecordedFrame
{
    std::uint32_t frameNumber = 0;
    std::uint64_t checksum = 0;
};

std::string goldenPath()
{
    return std::string{UNISON_GOLDEN_DIR} + "/arena_scripted.checksums";
}

std::vector<RecordedFrame> playAndRecord(std::uint32_t frames, std::uint32_t everyNthFrame)
{
    arena::ArenaSimulation match{unison::test::kScriptedPlayers};

    std::vector<RecordedFrame> recorded;

    for (std::uint32_t frameNumber = 0; frameNumber < frames; ++frameNumber)
    {
        match.advance(unison::test::scriptedInputs(frameNumber));

        if (match.frame().frameNumber % everyNthFrame == 0U)
        {
            recorded.push_back(RecordedFrame{match.frame().frameNumber, unison::sim::checksumOf(match.frame())});
        }
    }

    return recorded;
}

std::vector<RecordedFrame> readGolden()
{
    std::ifstream file{goldenPath()};
    std::vector<RecordedFrame> recorded;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line.front() == '#')
        {
            continue;
        }

        std::istringstream reader{line};
        RecordedFrame frame;

        reader >> frame.frameNumber >> std::hex >> frame.checksum;
        recorded.push_back(frame);
    }

    return recorded;
}

}

TEST_CASE("the scripted match is checked against every tenth frame that was recorded for it")
{
    const std::vector<RecordedFrame> golden = readGolden();

    REQUIRE(golden.size() == kFrames / kEveryNthFrame);

    const std::vector<RecordedFrame> played = playAndRecord(kFrames, kEveryNthFrame);

    REQUIRE(played.size() == golden.size());

    for (std::size_t index = 0; index < golden.size(); ++index)
    {
        REQUIRE(played[index].frameNumber == golden[index].frameNumber);

        if (played[index].checksum != golden[index].checksum)
        {
            FAIL("the match parted ways with what was recorded on frame " << golden[index].frameNumber);
        }
    }
}

TEST_CASE("recording the checksums of the scripted match", "[.record]")
{
    std::ofstream file{goldenPath()};

    file << "# the arena played on the scripted inputs of tests/support/arena_script.hpp\n";
    file << "# " << unison::test::kScriptedPlayers << " players, " << kFrames << " frames, every " << kEveryNthFrame
         << "th frame\n";
    file << "# re-recorded on purpose only: unison_tests_arena [.record]\n";

    for (const RecordedFrame& frame : playAndRecord(kFrames, kEveryNthFrame))
    {
        file << frame.frameNumber << ' ' << std::hex << std::setw(16) << std::setfill('0') << frame.checksum << std::dec
             << '\n';
    }

    REQUIRE(file.good());
}
