#pragma once

#include <arena/apply_input.hpp>
#include <arena/assets.hpp>
#include <arena/character_move.hpp>
#include <arena/hits.hpp>
#include <arena/lifetimes.hpp>
#include <arena/match_rules.hpp>
#include <arena/respawn.hpp>
#include <arena/weapons.hpp>

#include <unison/sim/frame.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/physics_step.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <cstddef>
#include <cstdint>

namespace arena
{

/// How many times a second a match ticks unless its host asks for another rate.
inline constexpr std::uint16_t kDefaultTickRate = 60;

/// A whole match, ready for a host to step: the assets it is built from, the frame its state lives
/// in, the systems that play it and the one order they run in. It owns all of them, so a host that
/// holds one has everything a tick needs and nothing else to assemble.
class ArenaSimulation
{
public:
    /// Sets a match up for the players, each tick a `tickRate`-th of a second long; a rate of nought breaks a
    /// contract.
    explicit ArenaSimulation(std::size_t players, std::uint16_t tickRate = kDefaultTickRate);

    ArenaSimulation(const ArenaSimulation&) = delete;
    ArenaSimulation& operator=(const ArenaSimulation&) = delete;
    ArenaSimulation(ArenaSimulation&&) = delete;
    ArenaSimulation& operator=(ArenaSimulation&&) = delete;

    /// Runs one tick of the match on the inputs of that tick.
    void advance(const unison::sim::FrameInputs& inputs);

    [[nodiscard]] unison::sim::Frame& frame();

    [[nodiscard]] const unison::sim::Frame& frame() const;

    [[nodiscard]] const unison::sim::AssetRegistry& assets() const;

    [[nodiscard]] const unison::sim::SystemPipeline& pipeline() const;

private:
    unison::sim::AssetRegistry assetTables;
    unison::sim::Frame liveFrame;
    ApplyInput applyInput;
    CharacterMove characterMove;
    Weapons weapons;
    unison::sim::PhysicsStep physicsStep;
    Hits hits;
    Lifetimes lifetimes;
    Respawn respawn;
    MatchRules matchRules;
    unison::sim::SystemPipeline systemPipeline;
};

}
