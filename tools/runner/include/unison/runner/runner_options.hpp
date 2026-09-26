#pragma once

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace unison::runner
{

/// What one run of the runner plays: how many clients for how many frames, from which seed, over how bad a
/// network (its loss as a share of one), how fast the match ticks, every how many verified frames the clients
/// compare checksums, the file the run is recorded into, none when empty, the folder the clients write desync dumps
/// into, none when empty, the client that starts with the first player one health point low, to show a desync, and the
/// frame the first client has verified when the last one joins, every client joining at the start when nought.
/// A run asked for help lists the options instead.
struct RunnerOptions
{
    std::uint32_t players = 2;
    std::uint32_t frames = 600;
    std::uint64_t seed = 1;
    std::uint32_t latencyMilliseconds = 0;
    std::uint32_t jitterMilliseconds = 0;
    float lossRate = 0.0F;
    std::uint16_t tickRate = 60;
    std::uint32_t checksumInterval = 1;
    std::string recordPath;
    std::string dumpDirectory = ".";
    std::optional<std::uint32_t> faultyClient;
    std::uint32_t lateJoinFrame = 0;
    bool isHelpAsked = false;
};

/// Reads the runner's command line, the program's name first. A value no run can play is refused with an error
/// naming the option: players outside one to eight, no frames, a loss outside nought to a hundred per cent,
/// a tick rate or a checksum interval of nought, a faulty client the match does not have, or a late join into a match
/// of one player or at a frame the run does not play before its last. An unknown option or a value that is no number
/// ends the program with the message the parser prints.
[[nodiscard]] tl::expected<RunnerOptions, Error> parseRunnerOptions(std::span<const char* const> arguments);

/// Every option of the runner and what it does, as `--help` prints them.
[[nodiscard]] std::string runnerHelp();

}
