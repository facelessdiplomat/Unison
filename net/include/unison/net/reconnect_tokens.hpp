#pragma once

#include <unison/core/rng.hpp>

#include <cstdint>

namespace unison::net
{

/// Draws the tokens a relay hands its players to win their slots back after a drop, never nought, which stands for no
/// token. A relay seeded from the operating system draws tokens nobody can guess; the same seed draws the same ones.
class ReconnectTokens
{
public:
    explicit ReconnectTokens(std::uint64_t seed);

    [[nodiscard]] std::uint64_t next();

private:
    Rng rng;
};

}
