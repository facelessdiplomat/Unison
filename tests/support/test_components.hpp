#pragma once

#include <cstdint>

namespace unison::test
{

/// Position of a test entity, a plain pair of floats with nothing between them.
struct Position
{
    float x = 0.0F;
    float y = 0.0F;
};

/// Remaining health of a test entity.
struct Health
{
    std::int32_t points = 0;
};

}
