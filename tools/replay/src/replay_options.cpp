#include <unison/replay/replay_options.hpp>

#include <cxxopts.hpp>

#include <string_view>

namespace unison::replay
{

namespace
{

cxxopts::Options describedOptions()
{
    cxxopts::Options options{
        "unison_replay",
        "Plays a replay back or verifies it, or compares two snapshots: play <file> re-simulates a "
        "replay, verify <file> also compares every checksum it recorded, diff <a> <b> names where "
        "two snapshots first differ."};

    cxxopts::OptionAdder add = options.add_options();

    add("command", "play, verify or diff", cxxopts::value<std::string>()->default_value(""));
    add("file", "the replay file, or the first snapshot", cxxopts::value<std::string>()->default_value(""));
    add("other", "the second snapshot of a diff", cxxopts::value<std::string>()->default_value(""));
    add("help", "lists these options");

    options.parse_positional({"command", "file", "other"});
    options.positional_help("play|verify <replay> | diff <snapshot> <snapshot>");

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
    read.otherFile = parsed["other"].as<std::string>();
    read.isHelpAsked = parsed.count("help") > 0;

    if (read.isHelpAsked)
    {
        return read;
    }

    const std::string command = parsed["command"].as<std::string>();

    if (command != "play" && command != "verify" && command != "diff")
    {
        return refused("the command is play, verify or diff");
    }

    if (read.file.empty() || (command == "diff" && read.otherFile.empty()))
    {
        return refused("a replay file follows play and verify, two snapshot files follow diff");
    }

    read.command = command == "play"   ? ReplayCommand::Play
                   : command == "diff" ? ReplayCommand::Diff
                                       : ReplayCommand::Verify;

    return read;
}

std::string replayHelp()
{
    return describedOptions().help();
}

}
