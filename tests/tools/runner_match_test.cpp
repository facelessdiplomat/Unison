#include <unison/runner/runner_match.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/net/session_config.hpp>
#include <unison/runner/client_outcome.hpp>
#include <unison/runner/runner_options.hpp>
#include <unison/session/rollback_stats.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

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

TEST_CASE("four players over a round trip of 240 ms keep pace with the host at 60 Hz")
{
    unison::runner::RunnerOptions options;
    options.players = 4;
    options.frames = 300;
    options.latencyMilliseconds = 120;
    options.jitterMilliseconds = 30;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE(outcome.hostFrames < options.frames * 6U / 5U);
}

TEST_CASE("a run waits for every client's checksum of every frame it verified and finds them alike")
{
    unison::runner::RunnerOptions options;
    options.frames = 120;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.framesCompared >= options.frames);
    REQUIRE_FALSE(outcome.disagreement.has_value());
}

TEST_CASE("a run over a late network reports rollbacks no deeper than the window it allowed")
{
    unison::runner::RunnerOptions options;
    options.frames = 120;
    options.latencyMilliseconds = 30;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();
    const unison::session::RollbackStats all = unison::runner::rollbacksOfAll(outcome.clients);

    REQUIRE(outcome.predictionWindow == unison::net::SessionConfig{}.maxPrediction);
    REQUIRE(all.deepestRollback > 0U);
    REQUIRE(all.deepestRollback <= outcome.predictionWindow);
}

TEST_CASE("every client of a run plays a slot of its own")
{
    unison::runner::RunnerOptions options;
    options.players = 3;
    options.frames = 60;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();
    std::vector<std::uint8_t> slots;

    for (const unison::runner::ClientOutcome& client : outcome.clients)
    {
        slots.push_back(client.slot);
    }

    std::ranges::sort(slots);

    REQUIRE(slots == std::vector<std::uint8_t>{0, 1, 2});
}

TEST_CASE("every client of a run reports the frames it played and the rollbacks it took")
{
    unison::runner::RunnerOptions options;
    options.frames = 120;
    options.latencyMilliseconds = 30;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.clients.size() == options.players);

    for (const unison::runner::ClientOutcome& client : outcome.clients)
    {
        REQUIRE(client.rollbacks.framesPlayed >= options.frames);
        REQUIRE(client.rollbacks.rollbacks > 0U);
    }
}
