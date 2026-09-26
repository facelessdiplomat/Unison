#pragma once

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <span>
#include <string>

namespace unison::replay
{

/// What the tool does with a replay: play it back, or play it back and compare its checksums.
enum class ReplayCommand : std::uint8_t
{
    Play,
    Verify
};

/// What one run of the replay tool does and with which replay file. A run asked for help lists the options instead.
struct ReplayOptions
{
    ReplayCommand command = ReplayCommand::Verify;
    std::string file;
    bool isHelpAsked = false;
};

/// Reads the tool's command line, the program's name first, then the command and the file. A command line without
/// both, or with a command the tool does not know, is refused with an error that says so.
[[nodiscard]] tl::expected<ReplayOptions, Error> parseReplayOptions(std::span<const char* const> arguments);

/// Every command and option of the tool and what it does, as `--help` prints them.
[[nodiscard]] std::string replayHelp();

}
