#include <arena/match_rules.hpp>

#include <arena/assets.hpp>
#include <arena/components.hpp>

#include <unison/core/fixed_vector.hpp>

#include <entt/entity/registry.hpp>

namespace arena
{

namespace
{

void countKills(unison::sim::Frame& frame, bool counting)
{
    unison::FixedVector<entt::entity, kPlayerCount> counted;

    for (const auto [entity, killed] : frame.registry.view<const Killed>().each())
    {
        counted.pushBack(entity);

        if (counting && frame.registry.valid(killed.by) && frame.registry.all_of<Score>(killed.by))
        {
            ++frame.registry.get<Score>(killed.by).kills;
        }
    }

    for (const entt::entity entity : counted)
    {
        frame.registry.erase<Killed>(entity);
    }
}

bool someoneHasWon(const unison::sim::Frame& frame, std::uint32_t killsToWin)
{
    for (const auto [entity, score] : frame.registry.view<const Score>().each())
    {
        if (score.kills >= killsToWin)
        {
            return true;
        }
    }

    return false;
}

}

MatchRules::MatchRules(const unison::sim::AssetRegistry& assets) : assets{assets}
{
}

void MatchRules::update(unison::sim::Frame& frame, const unison::sim::FrameInputs&)
{
    const MatchStats& stats = assets.get<MatchStats>(kMatchStats);

    if (frame.globals.matchPhase == unison::sim::MatchPhase::Warmup && frame.frameNumber >= stats.warmupFrames)
    {
        frame.globals.matchPhase = unison::sim::MatchPhase::Playing;
    }

    countKills(frame, frame.globals.matchPhase == unison::sim::MatchPhase::Playing);

    if (frame.globals.matchPhase == unison::sim::MatchPhase::Playing && someoneHasWon(frame, stats.killsToWin))
    {
        frame.globals.matchPhase = unison::sim::MatchPhase::Ended;
    }
}

std::string_view MatchRules::name() const
{
    return "MatchRules";
}

}
