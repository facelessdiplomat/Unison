#pragma once

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <span>
#include <string>

namespace unison::runner
{

/// What one run of the runner plays: how many clients for how many frames, from which seed, over how bad a
/// network, how fast the match ticks, every how many verified frames the clients compare checksums, and the
/// file the run is recorded into, none when empty. A run asked for help lists the options instead.
struct RunnerOptions
{
    std::uint32_t players = 2;
    std::uint32_t frames = 600;
    std::uint64_t seed = 1;
    std::uint32_t latencyMilliseconds = 0;
    std::uint32_t jitterMilliseconds = 0;
    float lossPercent = 0.0F;
    std::uint16_t tickRate = 60;
    std::uint32_t checksumInterval = 1;
    std::string recordPath;
    bool isHelpAsked = false;
};

/// Reads the runner's command line, the program's name first. A value no run can play is refused with an error
/// naming the option: players outside one to eight, no frames, a loss outside nought to a hundred per cent,
/// a tick rate or a checksum interval of nought. An unknown option or a value that is no number ends the
/// program with the message the parser prints.
[[nodiscard]] tl::expected<RunnerOptions, Error> parseRunnerOptions(std::span<const char* const> arguments);

/// Every option of the runner and what it does, as `--help` prints them.
[[nodiscard]] std::string runnerHelp();

}
