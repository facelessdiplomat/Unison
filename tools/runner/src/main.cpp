#include <unison/runner/runner_options.hpp>

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
    }

    return 0;
}
