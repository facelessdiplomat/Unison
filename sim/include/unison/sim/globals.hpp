#pragma once

#include <unison/core/rng.hpp>
#include <unison/sim/body_id_allocator.hpp>

#include <cstdint>

namespace unison::sim
{

/// Where a match stands, from the countdown before it starts to the scoreboard after it ends.
enum class MatchPhase : std::uint8_t
{
    Warmup,
    Playing,
    Ended
};

/// The frame state that belongs to no entity: everything a system needs about the match as a whole.
struct Globals
{
    Rng rng{};
    MatchPhase matchPhase = MatchPhase::Warmup;
    BodyIdAllocator bodyIds{};
};

}
