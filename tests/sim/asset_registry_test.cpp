#include <unison/sim/asset_registry.hpp>

#include <support/fatal_handler_probe.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace
{

struct BodyDefinition
{
    float halfExtent = 0.0F;
    float friction = 0.0F;
};

struct PlayerStats
{
    std::uint32_t maxHealth = 0;
    std::uint32_t speed = 0;
};

constexpr unison::AssetId kCrate = unison::makeAssetId("crate");
constexpr unison::AssetId kRamp = unison::makeAssetId("ramp");
constexpr unison::AssetId kSoldier = unison::makeAssetId("soldier");
constexpr unison::AssetId kUnknown = unison::makeAssetId("nothing at all");

unison::sim::AssetRegistry makeRegistry()
{
    unison::sim::AssetRegistry registry;

    registry.add(kCrate, BodyDefinition{0.5F, 0.8F});
    registry.add(kRamp, BodyDefinition{2.0F, 0.4F});
    registry.add(kSoldier, PlayerStats{100, 7});
    registry.freeze();

    return registry;
}

}

TEST_CASE("an asset registry hands back what was put into it")
{
    const unison::sim::AssetRegistry registry = makeRegistry();

    REQUIRE(registry.get<BodyDefinition>(kCrate).halfExtent == 0.5F);
    REQUIRE(registry.get<BodyDefinition>(kRamp).halfExtent == 2.0F);
    REQUIRE(registry.get<PlayerStats>(kSoldier).maxHealth == 100U);
}

TEST_CASE("a registry says when it has been frozen")
{
    unison::sim::AssetRegistry registry;

    REQUIRE_FALSE(registry.isFrozen());

    registry.freeze();

    REQUIRE(registry.isFrozen());
}

TEST_CASE("asking for an asset nobody registered breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    const unison::sim::AssetRegistry registry = makeRegistry();

    static_cast<void>(registry.get<BodyDefinition>(kUnknown).halfExtent);

    REQUIRE(probe.failureCount() > 0U);
}

TEST_CASE("asking for an asset of a type nobody registered breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::AssetRegistry registry;

    registry.add(kCrate, BodyDefinition{0.5F, 0.8F});
    registry.freeze();

    static_cast<void>(registry.get<PlayerStats>(kCrate).maxHealth);

    REQUIRE(probe.failureCount() > 0U);
}

TEST_CASE("adding an asset after freezing breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::AssetRegistry registry;

    registry.freeze();
    registry.add(kCrate, BodyDefinition{0.5F, 0.8F});

    REQUIRE(probe.failureCount() > 0U);
}

TEST_CASE("asking before freezing breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::AssetRegistry registry;

    registry.add(kCrate, BodyDefinition{0.5F, 0.8F});
    static_cast<void>(registry.get<BodyDefinition>(kCrate).halfExtent);

    REQUIRE(probe.failureCount() > 0U);
}

TEST_CASE("an identifier that is already taken breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::AssetRegistry registry;

    registry.add(kCrate, BodyDefinition{0.5F, 0.8F});
    registry.add(kCrate, BodyDefinition{1.0F, 0.2F});

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("an identifier taken by another type of asset breaks a contract too")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::AssetRegistry registry;

    registry.add(kCrate, BodyDefinition{0.5F, 0.8F});
    registry.add(kCrate, PlayerStats{100, 7});

    REQUIRE(probe.failureCount() == 1U);
}
