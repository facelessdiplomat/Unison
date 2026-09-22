#include <unison/sim/physics_layers.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("two static bodies never collide")
{
    REQUIRE_FALSE(unison::sim::layersCollide(unison::sim::PhysicsLayer::Static, unison::sim::PhysicsLayer::Static));
}

TEST_CASE("a moving body collides with anything")
{
    REQUIRE(unison::sim::layersCollide(unison::sim::PhysicsLayer::Moving, unison::sim::PhysicsLayer::Static));
    REQUIRE(unison::sim::layersCollide(unison::sim::PhysicsLayer::Static, unison::sim::PhysicsLayer::Moving));
    REQUIRE(unison::sim::layersCollide(unison::sim::PhysicsLayer::Moving, unison::sim::PhysicsLayer::Moving));
}

TEST_CASE("a layer and the numbers jolt stores name each other")
{
    REQUIRE(unison::sim::toPhysicsLayer(unison::sim::toObjectLayer(unison::sim::PhysicsLayer::Moving)) ==
            unison::sim::PhysicsLayer::Moving);
    REQUIRE(unison::sim::toPhysicsLayer(unison::sim::toBroadPhaseLayer(unison::sim::PhysicsLayer::Moving)) ==
            unison::sim::PhysicsLayer::Moving);
}

TEST_CASE("each layer gets a broad phase tree of its own")
{
    const unison::sim::BroadPhaseLayerMapping mapping;

    REQUIRE(mapping.GetNumBroadPhaseLayers() == unison::sim::kPhysicsLayerCount);
    REQUIRE(mapping.GetBroadPhaseLayer(unison::sim::toObjectLayer(unison::sim::PhysicsLayer::Static)) !=
            mapping.GetBroadPhaseLayer(unison::sim::toObjectLayer(unison::sim::PhysicsLayer::Moving)));
}

TEST_CASE("the layer rule reaches the filters jolt asks")
{
    const unison::sim::BroadPhaseLayerMapping mapping;
    const unison::sim::ObjectVsBroadPhaseFilter broadPhaseFilter;
    const unison::sim::ObjectPairFilter pairFilter;

    const JPH::ObjectLayer staticLayer = unison::sim::toObjectLayer(unison::sim::PhysicsLayer::Static);
    const JPH::ObjectLayer movingLayer = unison::sim::toObjectLayer(unison::sim::PhysicsLayer::Moving);

    REQUIRE_FALSE(pairFilter.ShouldCollide(staticLayer, staticLayer));
    REQUIRE(pairFilter.ShouldCollide(staticLayer, movingLayer));

    REQUIRE_FALSE(broadPhaseFilter.ShouldCollide(staticLayer, mapping.GetBroadPhaseLayer(staticLayer)));
    REQUIRE(broadPhaseFilter.ShouldCollide(staticLayer, mapping.GetBroadPhaseLayer(movingLayer)));
}
