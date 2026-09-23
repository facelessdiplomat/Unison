#include <unison/runner/runner_match.hpp>
#include <unison/runner/runner_options.hpp>

#include <arena/assets.hpp>

#include <cstddef>
#include <cstdio>
#include <span>

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

    if (!outcome.isComplete)
    {
        std::fprintf(stderr,
                     "unison_runner: the slowest client verified %u of %u frames in %u host frames\n",
                     outcome.fewestVerifiedFrames,
                     options->frames,
                     outcome.hostFrames);

        return 1;
    }

    std::printf("unison_runner: %u players verified %u frames in %u host frames\n",
                options->players,
                options->frames,
                outcome.hostFrames);

    return 0;
}
