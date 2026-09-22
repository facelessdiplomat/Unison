#include <unison/sim/physics_layer.hpp>

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
