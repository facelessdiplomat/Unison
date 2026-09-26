#include <unison/runner/run_outcome.hpp>
#include <unison/runner/runner_match.hpp>
#include <unison/runner/runner_options.hpp>

#include <arena/assets.hpp>

#include <unison/session/file_bytes.hpp>

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <span>
#include <string>

namespace
{

[[nodiscard]] bool keepRecording(const unison::runner::RunnerOptions& options, const unison::runner::RunnerMatch& match)
{
    if (options.recordPath.empty())
    {
        return true;
    }

    const tl::expected<void, unison::Error> written =
        unison::session::writeFileBytes(options.recordPath, match.replay());

    if (!written.has_value())
    {
        std::fprintf(stderr,
                     "unison_runner: %.*s: %s\n",
                     static_cast<int>(written.error().message().size()),
                     written.error().message().data(),
                     options.recordPath.c_str());

        return false;
    }

    std::printf("unison_runner: recorded the run into %s\n", options.recordPath.c_str());

    return true;
}

void reportDesyncDumps(const unison::runner::RunnerMatch& match)
{
    for (const tl::expected<std::filesystem::path, unison::Error>& dump : match.desyncDumps())
    {
        if (dump.has_value())
        {
            std::fprintf(stderr, "unison_runner: dumped the snapshot of the desync into %s\n", dump->string().c_str());
        }
        else
        {
            std::fprintf(stderr,
                         "unison_runner: %.*s\n",
                         static_cast<int>(dump.error().message().size()),
                         dump.error().message().data());
        }
    }
}

}

int main(int argc, char** argv)
{
    const tl::expected<unison::runner::RunnerOptions, unison::Error> options =
        unison::runner::parseRunnerOptions(std::span<const char* const>{argv, static_cast<std::size_t>(argc)});

    if (!options.has_value())
    {
        std::fprintf(stderr,
                     "unison_runner: %.*s\n",
                     static_cast<int>(options.error().message().size()),
                     options.error().message().data());

        return 1;
    }

    if (options->isHelpAsked)
    {
        std::fputs(unison::runner::runnerHelp().c_str(), stdout);

        return 0;
    }

    if (options->players > arena::kPlayerCount)
    {
        std::fprintf(stderr, "unison_runner: the arena holds %zu players at most\n", arena::kPlayerCount);

        return 1;
    }

    unison::runner::RunnerMatch match{*options};
    const unison::runner::RunOutcome outcome = match.play();
    const int exitCode = unison::runner::exitCodeOf(outcome);
    const std::string report = unison::runner::reportOf(outcome, *options);

    std::fputs(report.c_str(), exitCode == 0 ? stdout : stderr);
    reportDesyncDumps(match);

    if (!keepRecording(*options, match) && exitCode == 0)
    {
        return 1;
    }

    return exitCode;
}
