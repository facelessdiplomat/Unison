#pragma once

#include <arena/arena_input.hpp>

#include <unison/core/rng.hpp>

#include <cstdint>

namespace unison::runner
{

/// A player the runner plays by itself. It keeps a course, a way to walk and a way to face, for a while and
/// then picks another, and jumps and fires now and then; every choice is drawn from a generator of its own,
/// seeded from the run's seed and the player's number, so a run played again with one seed plays the same.
class ScriptedPlayer
{
public:
    ScriptedPlayer(std::uint64_t seed, std::uint32_t player);

    [[nodiscard]] arena::ArenaInput nextInput();

private:
    Rng rng;
    arena::ArenaInput course;
    std::int32_t framesLeftOnCourse = 0;
};

}
