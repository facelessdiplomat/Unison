#include <unison/runner/runner_match.hpp>

#include <arena/arena_simulation.hpp>

#include <support/replay_playback.hpp>
#include <unison/runner/run_outcome.hpp>
#include <unison/runner/runner_options.hpp>
#include <unison/session/verified_checksum.hpp>

#include <catch2/catch_test_macros.hpp>

#include <vector>

TEST_CASE("a replay the runner recorded plays back to the checksums it recorded")
{
    unison::runner::RunnerOptions options;
    options.players = 2;
    options.frames = 240;
    options.latencyMilliseconds = 40;
    options.jitterMilliseconds = 10;
    options.lossRate = 0.05F;
    options.checksumInterval = 10;
    options.recordPath = "match.replay";
    unison::runner::RunnerMatch match{options};
    REQUIRE(match.play().isComplete);
    arena::ArenaSimulation replayed{options.players, options.tickRate};

    const std::vector<unison::session::VerifiedChecksum> recorded = unison::test::recordedChecksums(match.replay());

    REQUIRE(recorded.size() >= options.frames / options.checksumInterval);
    REQUIRE(unison::test::playedChecksums(match.replay(), replayed.frame(), replayed.pipeline()) == recorded);
}
