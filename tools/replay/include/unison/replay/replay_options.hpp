#pragma once

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <span>
#include <string>

namespace unison::replay
{

/// What the tool does: play a replay back, play it back and compare its checksums, or compare two snapshots.
enum class ReplayCommand : std::uint8_t
{
    Play,
    Verify,
    Diff
};

/// What one run of the replay tool does and with which file, and with which other one for a diff. A run asked for help
/// lists the options instead.
struct ReplayOptions
{
    ReplayCommand command = ReplayCommand::Verify;
    std::string file;
    std::string otherFile;
    bool isHelpAsked = false;
};

/// Reads the tool's command line, the program's name first, then the command and its files: one replay, or two
/// snapshots for a diff. A command line without them, or with a command the tool does not know, is refused.
[[nodiscard]] tl::expected<ReplayOptions, Error> parseReplayOptions(std::span<const char* const> arguments);

/// Every command and option of the tool and what it does, as `--help` prints them.
[[nodiscard]] std::string replayHelp();

}
