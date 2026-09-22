#include <unison/sim/asset_hash.hpp>

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

}

TEST_CASE("registries holding the same assets hash alike whatever order they were filled in")
{
    unison::sim::AssetRegistry oneOrder;
    oneOrder.add(kCrate, BodyDefinition{0.5F, 0.8F});
    oneOrder.add(kSoldier, PlayerStats{100, 7});
    oneOrder.add(kRamp, BodyDefinition{2.0F, 0.4F});
    oneOrder.freeze();

    unison::sim::AssetRegistry otherOrder;
    otherOrder.add(kRamp, BodyDefinition{2.0F, 0.4F});
    otherOrder.add(kCrate, BodyDefinition{0.5F, 0.8F});
    otherOrder.add(kSoldier, PlayerStats{100, 7});
    otherOrder.freeze();

    REQUIRE(unison::sim::hashOf(oneOrder) == unison::sim::hashOf(otherOrder));
}

TEST_CASE("changing what an asset holds changes the hash")
{
    unison::sim::AssetRegistry before;
    before.add(kCrate, BodyDefinition{0.5F, 0.8F});
    before.freeze();

    unison::sim::AssetRegistry after;
    after.add(kCrate, BodyDefinition{0.5F, 0.9F});
    after.freeze();

    REQUIRE(unison::sim::hashOf(before) != unison::sim::hashOf(after));
}

TEST_CASE("adding an asset changes the hash")
{
    unison::sim::AssetRegistry smaller;
    smaller.add(kCrate, BodyDefinition{0.5F, 0.8F});
    smaller.freeze();

    unison::sim::AssetRegistry larger;
    larger.add(kCrate, BodyDefinition{0.5F, 0.8F});
    larger.add(kRamp, BodyDefinition{2.0F, 0.4F});
    larger.freeze();

    REQUIRE(unison::sim::hashOf(smaller) != unison::sim::hashOf(larger));
}

TEST_CASE("renaming an asset changes the hash")
{
    unison::sim::AssetRegistry named;
    named.add(kCrate, BodyDefinition{0.5F, 0.8F});
    named.freeze();

    unison::sim::AssetRegistry renamed;
    renamed.add(kRamp, BodyDefinition{0.5F, 0.8F});
    renamed.freeze();

    REQUIRE(unison::sim::hashOf(named) != unison::sim::hashOf(renamed));
}

TEST_CASE("hashing a registry that was never frozen breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::AssetRegistry registry;

    registry.add(kCrate, BodyDefinition{0.5F, 0.8F});
    static_cast<void>(unison::sim::hashOf(registry));

    REQUIRE(probe.failureCount() == 1U);
}
