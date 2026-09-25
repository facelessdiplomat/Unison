#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/net/session_config.hpp>
#include <unison/sim/asset_hash.hpp>
#include <unison/sim/pipeline_hash.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

namespace
{

constexpr std::array<std::uint8_t, 3> kPlayerCounts{2, 4, 8};

struct ConfigHashes
{
    std::uint32_t players = 0;
    std::uint64_t assetHash = 0;
    std::uint64_t pipelineHash = 0;
    std::uint64_t configHash = 0;
};

std::string goldenPath()
{
    return std::string{UNISON_GOLDEN_DIR} + "/arena_config.hashes";
}

ConfigHashes hashesFor(std::uint8_t players)
{
    const arena::ArenaSimulation match{players};

    unison::net::SessionConfig config;
    config.slotCount = players;
    config.inputSize = sizeof(arena::ArenaInput);
    config.seed = 1;
    config.assetHash = unison::sim::hashOf(match.assets());
    config.pipelineHash = unison::sim::hashOf(match.pipeline());

    return ConfigHashes{players, config.assetHash, config.pipelineHash, unison::net::hashOf(config)};
}

std::vector<ConfigHashes> readGolden()
{
    std::ifstream file{goldenPath()};
    std::vector<ConfigHashes> recorded;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line.front() == '#')
        {
            continue;
        }

        std::istringstream reader{line};
        ConfigHashes hashes;

        reader >> hashes.players >> std::hex >> hashes.assetHash >> hashes.pipelineHash >> hashes.configHash;
        recorded.push_back(hashes);
    }

    return recorded;
}

}

TEST_CASE("the arena's session config hashes as recorded so that two machines meet in one relay room")
{
    const std::vector<ConfigHashes> golden = readGolden();

    REQUIRE(golden.size() == kPlayerCounts.size());

    for (std::size_t index = 0; index < golden.size(); ++index)
    {
        const ConfigHashes built = hashesFor(kPlayerCounts[index]);

        REQUIRE(built.players == golden[index].players);
        REQUIRE(built.assetHash == golden[index].assetHash);
        REQUIRE(built.pipelineHash == golden[index].pipelineHash);
        REQUIRE(built.configHash == golden[index].configHash);
    }
}

TEST_CASE("recording the arena's session config hashes", "[.record]")
{
    std::ofstream file{goldenPath()};

    file << "# the arena's session config as its console builds it: players, then its asset, pipeline and config "
            "hashes\n";
    file << "# re-recorded on purpose only: unison_tests_arena \"recording the arena's session config hashes\"\n";

    for (const std::uint8_t players : kPlayerCounts)
    {
        const ConfigHashes hashes = hashesFor(players);

        file << hashes.players << std::hex << std::setfill('0') << ' ' << std::setw(16) << hashes.assetHash << ' '
             << std::setw(16) << hashes.pipelineHash << ' ' << std::setw(16) << hashes.configHash << std::dec << '\n';
    }

    REQUIRE(file.good());
}
