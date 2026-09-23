#include <unison/runner/runner_match.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/runner/runner_options.hpp>

TEST_CASE("two players over a flawless network verify every frame of the run")
{
    const unison::runner::RunnerOptions options;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE(outcome.fewestVerifiedFrames >= options.frames);
}

TEST_CASE("three players over a late, jittery and lossy network still verify every frame of the run")
{
    unison::runner::RunnerOptions options;
    options.players = 3;
    options.frames = 300;
    options.latencyMilliseconds = 50;
    options.jitterMilliseconds = 10;
    options.lossRate = 0.05F;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE(outcome.fewestVerifiedFrames >= options.frames);
}
