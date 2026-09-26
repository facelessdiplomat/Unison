#include <unison/replay/replay_options.hpp>
#include <unison/replay/replay_report.hpp>

#include <arena/arena_simulation.hpp>

#include <unison/session/file_bytes.hpp>
#include <unison/session/replay_player.hpp>
#include <unison/session/replay_reader.hpp>
#include <unison/session/replay_verification.hpp>
#include <unison/session/state_diff.hpp>
#include <unison/sim/asset_hash.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/pipeline_hash.hpp>

#include <cstddef>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace
{

constexpr int kUnreadable = 1;

int refuse(const unison::Error& error)
{
    std::fprintf(stderr, "unison_replay: %.*s\n", static_cast<int>(error.message().size()), error.message().data());

    return kUnreadable;
}

int playBack(const unison::replay::ReplayOptions& options, unison::session::ReplayReader& reader)
{
    arena::ArenaSimulation match{reader.config().slotCount, reader.config().tickRate};
    const tl::expected<void, unison::Error> content = unison::replay::checkRecordedContent(
        reader.config(), unison::sim::hashOf(match.assets()), unison::sim::hashOf(match.pipeline()));

    if (!content.has_value())
    {
        return refuse(content.error());
    }

    unison::session::ReplayPlayer player{match.frame(), match.pipeline(), reader.config()};
    const tl::expected<unison::session::ReplayVerdict, unison::Error> verdict =
        unison::session::verifyReplay(reader, player);

    if (!verdict.has_value())
    {
        return refuse(verdict.error());
    }

    if (options.command == unison::replay::ReplayCommand::Play)
    {
        std::fputs(unison::replay::playReportOf(*verdict, unison::sim::checksumOf(match.frame())).c_str(), stdout);

        return 0;
    }

    const int exitCode = unison::replay::verifyExitCodeOf(*verdict);
    std::fputs(unison::replay::verifyReportOf(*verdict).c_str(), exitCode == 0 ? stdout : stderr);

    return exitCode;
}

int diff(const unison::replay::ReplayOptions& options)
{
    const tl::expected<std::vector<std::byte>, unison::Error> first = unison::session::readFileBytes(options.file);
    const tl::expected<std::vector<std::byte>, unison::Error> second =
        unison::session::readFileBytes(options.otherFile);

    if (!first.has_value() || !second.has_value())
    {
        return refuse(first.has_value() ? second.error() : first.error());
    }

    const tl::expected<std::optional<unison::session::StateDifference>, unison::Error> difference =
        unison::session::firstDifferenceOf(*first, *second);

    if (!difference.has_value())
    {
        return refuse(difference.error());
    }

    std::fputs(unison::replay::diffReportOf(*difference).c_str(), stdout);

    return unison::replay::diffExitCodeOf(*difference);
}

}

int main(int argc, char** argv)
{
    const tl::expected<unison::replay::ReplayOptions, unison::Error> options =
        unison::replay::parseReplayOptions(std::span<const char* const>{argv, static_cast<std::size_t>(argc)});

    if (!options.has_value())
    {
        return refuse(options.error());
    }

    if (options->isHelpAsked)
    {
        std::fputs(unison::replay::replayHelp().c_str(), stdout);

        return 0;
    }

    if (options->command == unison::replay::ReplayCommand::Diff)
    {
        return diff(*options);
    }

    const tl::expected<std::vector<std::byte>, unison::Error> bytes = unison::session::readFileBytes(options->file);

    if (!bytes.has_value())
    {
        return refuse(bytes.error());
    }

    tl::expected<unison::session::ReplayReader, unison::Error> reader = unison::session::ReplayReader::open(*bytes);

    if (!reader.has_value())
    {
        return refuse(reader.error());
    }

    return playBack(*options, *reader);
}
