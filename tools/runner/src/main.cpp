#include <unison/runner/run_outcome.hpp>
#include <unison/runner/runner_match.hpp>
#include <unison/runner/runner_options.hpp>

#include <arena/assets.hpp>

#include <cstddef>
#include <cstdio>
#include <span>
#include <string>

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

    return exitCode;
}
