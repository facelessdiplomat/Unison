#include <arena/assets.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/asset_hash.hpp>
#include <unison/sim/body_definition.hpp>

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

TEST_CASE("what the arena is built from is plain data a hash can cover")
{
    STATIC_REQUIRE(unison::sim::AssetTraits<arena::SpawnPoints>::isValid);
    STATIC_REQUIRE(unison::sim::AssetTraits<arena::PlayerStats>::isValid);
    STATIC_REQUIRE(unison::sim::AssetTraits<arena::ProjectileStats>::isValid);
}
