#include <unison/runner/rollback_summary.hpp>

#include <unison/net/protocol.hpp>
#include <unison/runner/client_outcome.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace
{

constexpr std::uint16_t kTickRate = 60;

unison::runner::ClientOutcome clientInSlot(
    std::uint8_t slot, std::uint32_t rollbacks, std::uint64_t resimulated, std::uint32_t deepest, std::uint32_t stalled)
{
    unison::runner::ClientOutcome client;
    client.slot = slot;
    client.rollbacks.rollbacks = rollbacks;
    client.rollbacks.resimulatedFrames = resimulated;
    client.rollbacks.deepestRollback = deepest;
    client.rollbacks.stalledTicks = stalled;
    client.rollbacks.framesPlayed = 600;

    return client;
}

std::vector<std::vector<std::string>> rowsOf(const std::string& summary)
{
    std::istringstream lines{summary};
    std::vector<std::vector<std::string>> rows;

    for (std::string line; std::getline(lines, line);)
    {
        std::istringstream words{line};
        std::vector<std::string> row;

        for (std::string word; words >> word;)
        {
            row.push_back(word);
        }

        rows.push_back(row);
    }

    return rows;
}

std::vector<std::string> rowStartingWith(const std::string& summary, const std::string& first)
{
    for (const std::vector<std::string>& row : rowsOf(summary))
    {
        if (!row.empty() && row.front() == first)
        {
            return row;
        }
    }

    return {};
}

std::vector<std::string> firstWordsOf(const std::string& summary)
{
    std::vector<std::string> firstWords;

    for (const std::vector<std::string>& row : rowsOf(summary))
    {
        firstWords.push_back(row.empty() ? std::string{} : row.front());
    }

    return firstWords;
}

}

TEST_CASE("the rollback summary gives every slot its rollbacks, their rate, mean and deepest depth and its stalls")
{
    const std::array clients{clientInSlot(1, 12, 30, 6, 9), clientInSlot(0, 24, 36, 3, 0)};

    const std::string summary = unison::runner::rollbackSummaryOf(clients, kTickRate);

    REQUIRE(rowStartingWith(summary, "1") == std::vector<std::string>{"1", "12", "1.20", "2.50", "6", "9"});
    REQUIRE(rowStartingWith(summary, "0") == std::vector<std::string>{"0", "24", "2.40", "1.50", "3", "0"});
}

TEST_CASE("the rollback summary lists the slots in order")
{
    const std::array clients{clientInSlot(2, 1, 1, 1, 0), clientInSlot(0, 1, 1, 1, 0), clientInSlot(1, 1, 1, 1, 0)};

    const std::string summary = unison::runner::rollbackSummaryOf(clients, kTickRate);

    REQUIRE(firstWordsOf(summary) == std::vector<std::string>{"unison_runner:", "slot", "0", "1", "2", "all"});
}

TEST_CASE("the rollback summary closes with a row for every client together")
{
    const std::array clients{clientInSlot(1, 12, 30, 6, 9), clientInSlot(0, 24, 36, 3, 0)};

    const std::string summary = unison::runner::rollbackSummaryOf(clients, kTickRate);

    REQUIRE(rowStartingWith(summary, "all") == std::vector<std::string>{"all", "36", "1.80", "1.83", "6", "9"});
}

TEST_CASE("the rollback summary names a spectator's row after it, below the players' slots")
{
    const std::array clients{clientInSlot(unison::net::kNoSlot, 0, 0, 0, 4), clientInSlot(0, 24, 36, 3, 0)};

    const std::string summary = unison::runner::rollbackSummaryOf(clients, kTickRate);

    REQUIRE(firstWordsOf(summary) == std::vector<std::string>{"unison_runner:", "slot", "0", "spectator", "all"});
    REQUIRE(rowStartingWith(summary, "spectator") ==
            std::vector<std::string>{"spectator", "0", "0.00", "0.00", "0", "4"});
}
