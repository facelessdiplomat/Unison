#include <arena/assets.hpp>
#include <arena/facing.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <unison/sim/asset_hash.hpp>
#include <unison/sim/body_definition.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>

TEST_CASE("the arena freezes once it is defined")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    REQUIRE(assets.isFrozen());
    REQUIRE(assets.holds(arena::kFloor));
    REQUIRE(assets.holds(arena::kWall));
    REQUIRE(assets.holds(arena::kRamp));
    REQUIRE(assets.holds(arena::kCrate));
    REQUIRE(assets.holds(arena::kSpawnPoints));
    REQUIRE(assets.holds(arena::kPlayerStats));
    REQUIRE(assets.holds(arena::kProjectileStats));
}

TEST_CASE("two arenas defined the same way hash the same")
{
    unison::sim::AssetRegistry one;
    unison::sim::AssetRegistry other;

    arena::defineArena(one);
    arena::defineArena(other);

    REQUIRE(unison::sim::hashOf(one) == unison::sim::hashOf(other));
}

TEST_CASE("the arena gives every player somewhere to come in")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::SpawnPoints& points = assets.get<arena::SpawnPoints>(arena::kSpawnPoints);

    REQUIRE(points.positions.size() == arena::kPlayerCount);
    REQUIRE(points.positions[0].y > 0.0F);
}

TEST_CASE("every spawn point faces the middle of the arena")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::SpawnPoints& points = assets.get<arena::SpawnPoints>(arena::kSpawnPoints);

    for (std::size_t point = 0; point < arena::kPlayerCount; ++point)
    {
        CAPTURE(point);

        const unison::Float3 facing = arena::facingOf(points.yaws[point]);
        const unison::Float3& position = points.positions[point];
        const float distanceToMiddle = std::sqrt(position.x * position.x + position.z * position.z);
        const float alignment = -(facing.x * position.x + facing.z * position.z) / distanceToMiddle;

        REQUIRE_THAT(alignment, Catch::Matchers::WithinAbs(1.0, 0.0001));
    }
}

TEST_CASE("what the arena is built from is plain data a hash can cover")
{
    STATIC_REQUIRE(unison::sim::AssetTraits<arena::SpawnPoints>::isValid);
    STATIC_REQUIRE(unison::sim::AssetTraits<arena::PlayerStats>::isValid);
    STATIC_REQUIRE(unison::sim::AssetTraits<arena::ProjectileStats>::isValid);
}
