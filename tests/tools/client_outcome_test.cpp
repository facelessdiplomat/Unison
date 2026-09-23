#include <unison/runner/client_outcome.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>

namespace
{

unison::runner::ClientOutcome clientWith(std::uint32_t rollbacks, std::uint32_t deepest, std::uint32_t stalled)
{
    unison::runner::ClientOutcome client;
    client.rollbacks.rollbacks = rollbacks;
    client.rollbacks.deepestRollback = deepest;
    client.rollbacks.resimulatedFrames = rollbacks * 2U;
    client.rollbacks.stalledTicks = stalled;
    client.rollbacks.framesPlayed = 600;

    return client;
}

}

TEST_CASE("the rollbacks of every client together add up their counts and keep the deepest")
{
    const std::array clients{clientWith(10, 4, 1), clientWith(30, 7, 2), clientWith(20, 5, 3)};

    const unison::session::RollbackStats all = unison::runner::rollbacksOfAll(clients);

    REQUIRE(all.rollbacks == 60U);
    REQUIRE(all.deepestRollback == 7U);
    REQUIRE(all.resimulatedFrames == 120U);
    REQUIRE(all.stalledTicks == 6U);
    REQUIRE(all.framesPlayed == 1800U);
}

TEST_CASE("a run without clients has no rollbacks")
{
    const unison::session::RollbackStats all = unison::runner::rollbacksOfAll({});

    REQUIRE(all.rollbacks == 0U);
    REQUIRE(all.deepestRollback == 0U);
}
