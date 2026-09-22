#include <arena/match_rules.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/assets.hpp>
#include <arena/components.hpp>
#include <unison/sim/advance_frame.hpp>

#include <entt/entity/registry.hpp>

#include <cstdint>

namespace
{

entt::entity joinMatch(unison::sim::Frame& frame, std::uint8_t slot)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<arena::PlayerSlot>(entity, slot);
    frame.registry.emplace<arena::Score>(entity);

    return entity;
}

void run(unison::sim::Frame& frame, const unison::sim::SystemPipeline& pipeline, std::uint32_t ticks)
{
    for (std::uint32_t tick = 0; tick < ticks; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});
    }
}

}

TEST_CASE("a match warms up before it is played")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::MatchStats& stats = assets.get<arena::MatchStats>(arena::kMatchStats);

    arena::MatchRules rules{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(rules);

    unison::sim::Frame frame;

    run(frame, pipeline, stats.warmupFrames);

    REQUIRE(frame.globals.matchPhase == unison::sim::MatchPhase::Warmup);
    REQUIRE(frame.frameNumber == stats.warmupFrames);

    run(frame, pipeline, 1U);

    REQUIRE(frame.globals.matchPhase == unison::sim::MatchPhase::Playing);
}

TEST_CASE("a kill counts for the player who made it")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::MatchStats& stats = assets.get<arena::MatchStats>(arena::kMatchStats);

    arena::MatchRules rules{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(rules);

    unison::sim::Frame frame;
    const entt::entity killer = joinMatch(frame, 0U);
    const entt::entity victim = joinMatch(frame, 1U);

    run(frame, pipeline, stats.warmupFrames);

    frame.registry.emplace<arena::Killed>(victim, killer);

    run(frame, pipeline, 1U);

    REQUIRE(frame.registry.get<arena::Score>(killer).kills == 1U);
    REQUIRE(frame.registry.get<arena::Score>(victim).kills == 0U);
    REQUIRE_FALSE(frame.registry.all_of<arena::Killed>(victim));
}

TEST_CASE("a kill made before the match is played counts for nobody")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    arena::MatchRules rules{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(rules);

    unison::sim::Frame frame;
    const entt::entity killer = joinMatch(frame, 0U);
    const entt::entity victim = joinMatch(frame, 1U);

    frame.registry.emplace<arena::Killed>(victim, killer);

    run(frame, pipeline, 1U);

    REQUIRE(frame.registry.get<arena::Score>(killer).kills == 0U);
    REQUIRE_FALSE(frame.registry.all_of<arena::Killed>(victim));
}

TEST_CASE("a kill made by a player who has left counts for nobody")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::MatchStats& stats = assets.get<arena::MatchStats>(arena::kMatchStats);

    arena::MatchRules rules{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(rules);

    unison::sim::Frame frame;
    const entt::entity killer = joinMatch(frame, 0U);
    const entt::entity victim = joinMatch(frame, 1U);

    run(frame, pipeline, stats.warmupFrames);

    frame.registry.destroy(killer);
    frame.registry.emplace<arena::Killed>(victim, killer);

    run(frame, pipeline, 1U);

    REQUIRE_FALSE(frame.registry.all_of<arena::Killed>(victim));
}

TEST_CASE("the match ends when a player reaches the limit")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::MatchStats& stats = assets.get<arena::MatchStats>(arena::kMatchStats);

    arena::MatchRules rules{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(rules);

    unison::sim::Frame frame;
    const entt::entity leader = joinMatch(frame, 0U);
    const entt::entity other = joinMatch(frame, 1U);

    run(frame, pipeline, stats.warmupFrames);

    frame.registry.get<arena::Score>(leader).kills = stats.killsToWin - 1U;
    frame.registry.emplace<arena::Killed>(other, leader);

    run(frame, pipeline, 1U);

    REQUIRE(frame.registry.get<arena::Score>(leader).kills == stats.killsToWin);
    REQUIRE(frame.globals.matchPhase == unison::sim::MatchPhase::Ended);
}

TEST_CASE("a kill made after the match has ended counts for nobody")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::MatchStats& stats = assets.get<arena::MatchStats>(arena::kMatchStats);

    arena::MatchRules rules{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(rules);

    unison::sim::Frame frame;
    const entt::entity leader = joinMatch(frame, 0U);
    const entt::entity other = joinMatch(frame, 1U);

    run(frame, pipeline, stats.warmupFrames);

    frame.registry.get<arena::Score>(leader).kills = stats.killsToWin;

    run(frame, pipeline, 1U);

    REQUIRE(frame.globals.matchPhase == unison::sim::MatchPhase::Ended);

    frame.registry.emplace<arena::Killed>(other, leader);

    run(frame, pipeline, 1U);

    REQUIRE(frame.registry.get<arena::Score>(leader).kills == stats.killsToWin);
}
