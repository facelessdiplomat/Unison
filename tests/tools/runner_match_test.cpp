#include <unison/runner/runner_match.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/net/session_config.hpp>
#include <unison/runner/client_outcome.hpp>
#include <unison/runner/runner_options.hpp>
#include <unison/session/replay_reader.hpp>
#include <unison/session/rollback_stats.hpp>
#include <unison/sim/frame_inputs.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <variant>
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

TEST_CASE("eight players over a flawless network verify every frame of the run")
{
    unison::runner::RunnerOptions options;
    options.players = 8;
    options.frames = 120;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE_FALSE(outcome.disagreement.has_value());
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

TEST_CASE("four players losing a fifth of their messages over a round trip of 240 ms keep pace at 60 Hz")
{
    unison::runner::RunnerOptions options;
    options.players = 4;
    options.frames = 300;
    options.latencyMilliseconds = 120;
    options.jitterMilliseconds = 30;
    options.lossRate = 0.2F;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE(outcome.hostFrames < options.frames * 6U / 5U);
}

TEST_CASE("four players whose messages stray by up to 60 ms either way keep to the host's clock")
{
    unison::runner::RunnerOptions options;
    options.players = 4;
    options.frames = 1200;
    options.latencyMilliseconds = 120;
    options.jitterMilliseconds = 60;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE(outcome.hostFrames >= options.frames);
    REQUIRE(outcome.hostFrames < options.frames + options.tickRate);
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

TEST_CASE("a player joining a run late starts from a snapshot, verifies every frame and agrees with the others")
{
    unison::runner::RunnerOptions options;
    options.players = 3;
    options.frames = 300;
    options.lateJoinFrame = 100;
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE_FALSE(outcome.disagreement.has_value());
    REQUIRE(outcome.clients.back().startFrame > options.lateJoinFrame);
    REQUIRE(outcome.clients.front().startFrame == 0U);
    REQUIRE(outcome.framesCompared >= options.frames - outcome.clients.back().startFrame);
}

TEST_CASE("a player that drops and comes back with its token plays on in its slot and agrees with the others")
{
    unison::runner::RunnerOptions options;
    options.players = 3;
    options.frames = 300;
    options.drop = unison::runner::Drop{1, 100, 1};
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE_FALSE(outcome.disagreement.has_value());
    REQUIRE(outcome.clients[1].hasComeBack);
    REQUIRE(outcome.clients[1].slot == 1U);
    REQUIRE(outcome.clients[1].startFrame > options.drop->frame + options.tickRate - 10U);
    REQUIRE_FALSE(outcome.clients[0].hasComeBack);
}

TEST_CASE("nobody plays the slot of a player while it is away, which the others see dropped")
{
    unison::runner::RunnerOptions options;
    options.frames = 300;
    options.recordPath = "match.replay";
    options.drop = unison::runner::Drop{1, 100, 1};
    unison::runner::RunnerMatch match{options};
    REQUIRE(match.play().isComplete);
    auto reader = unison::session::ReplayReader::open(match.replay());
    REQUIRE(reader.has_value());
    std::optional<unison::sim::InputFlags> awayFlags;

    while (!reader->isAtEnd() && !awayFlags.has_value())
    {
        const auto record = reader->next();
        REQUIRE(record.has_value());
        const auto* frame = std::get_if<unison::session::ReplayFrame>(&*record);

        if (frame != nullptr && frame->frameNumber == options.drop->frame + 30U)
        {
            awayFlags = frame->inputs.flagsAt(1);
        }
    }

    REQUIRE(awayFlags == unison::sim::InputFlags::Dropped);
}

TEST_CASE("the others wait for nothing from a player while it is away, their frames confirmed as fast as before")
{
    constexpr std::uint32_t kDeepestOnAFlawlessNetwork = 3;
    unison::runner::RunnerOptions options;
    options.players = 3;
    options.frames = 300;
    options.drop = unison::runner::Drop{1, 100, 1};
    unison::runner::RunnerMatch match{options};

    const unison::runner::RunOutcome outcome = match.play();

    REQUIRE(outcome.isComplete);
    REQUIRE(outcome.hostFrames < options.frames + 20U);
    REQUIRE(outcome.clients[0].rollbacks.deepestRollback <= kDeepestOnAFlawlessNetwork);
    REQUIRE(outcome.clients[2].rollbacks.deepestRollback <= kDeepestOnAFlawlessNetwork);
}
