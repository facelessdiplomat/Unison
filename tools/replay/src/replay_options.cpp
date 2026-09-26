#include <unison/replay/replay_options.hpp>

#include <cxxopts.hpp>

#include <string_view>

namespace unison::replay
{

namespace
{

cxxopts::Options describedOptions()
{
    cxxopts::Options options{"unison_replay",
                             "Plays a replay back, or verifies it: play <file> re-simulates it, verify <file> also "
                             "compares every checksum it recorded."};

    cxxopts::OptionAdder add = options.add_options();

    add("command", "play or verify", cxxopts::value<std::string>()->default_value(""));
    add("file", "the replay file", cxxopts::value<std::string>()->default_value(""));
    add("help", "lists these options");

    options.parse_positional({"command", "file"});
    options.positional_help("play|verify <file>");

    return options;
}

tl::expected<ReplayOptions, Error> refused(std::string_view why)
{
    return tl::unexpected{Error{ErrorCode::InvalidOption, why}};
}

}

tl::expected<ReplayOptions, Error> parseReplayOptions(std::span<const char* const> arguments)
{
    cxxopts::Options options = describedOptions();
    const cxxopts::ParseResult parsed = options.parse(static_cast<int>(arguments.size()), arguments.data());

    ReplayOptions read;
    read.file = parsed["file"].as<std::string>();
    read.isHelpAsked = parsed.count("help") > 0;

    if (read.isHelpAsked)
    {
        return read;
    }

    const std::string command = parsed["command"].as<std::string>();

    if (command != "play" && command != "verify")
    {
        return refused("the command is play or verify");
    }

    if (read.file.empty())
    {
        return refused("a replay file follows the command");
    }

    read.command = command == "play" ? ReplayCommand::Play : ReplayCommand::Verify;

    return read;
}

std::string replayHelp()
{
    return describedOptions().help();
}

}
