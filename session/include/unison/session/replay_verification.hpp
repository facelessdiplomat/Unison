#pragma once

#include <unison/core/error.hpp>
#include <unison/session/replay_player.hpp>
#include <unison/session/replay_reader.hpp>

#include <tl/expected.hpp>

#include <cstdint>

namespace unison::session
{

/// What playing a replay back found: how many frames it played, how many recorded checksums it compared with
/// the ones those frames played to, and how many of them matched.
struct ReplayVerdict
{
    std::uint32_t framesPlayed = 0;
    std::uint32_t checksumsCompared = 0;
    std::uint32_t checksumsMatched = 0;
};

/// Plays every frame the reader has left through the player and compares every checksum it reads with the one the
/// player took of the frame just played. A checksum of any other frame is a malformed replay, and a record the
/// reader or a frame the player refuses stops it with that error.
[[nodiscard]] tl::expected<ReplayVerdict, Error> verifyReplay(ReplayReader& reader, ReplayPlayer& player);

}
