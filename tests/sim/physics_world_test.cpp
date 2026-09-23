#include <unison/sim/physics_world.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstddef>
#include <span>

#include <catch2/catch_test_macros.hpp>

namespace
{

constexpr float kTickSeconds = 1.0F / 60.0F;

}

TEST_CASE("a new physics world holds no bodies")
{
    const unison::sim::PhysicsWorld world;

    REQUIRE(world.bodies().count() == 0U);
}

TEST_CASE("an empty physics world steps a hundred times")
{
    unison::sim::PhysicsWorld world;

    for (int tick = 0; tick < 100; ++tick)
    {
        world.step(kTickSeconds);
    }

    REQUIRE(world.bodies().count() == 0U);
}

TEST_CASE("a physics world pulls bodies down the y axis in metres")
{
    const unison::sim::PhysicsWorld world;

    REQUIRE(world.gravity().x == 0.0F);
    REQUIRE(world.gravity().y == -9.81F);
    REQUIRE(world.gravity().z == 0.0F);
}

TEST_CASE("a physics world takes the gravity its settings ask for")
{
    unison::sim::PhysicsWorldSettings settings;
    settings.gravity = unison::Float3{0.0F, -1.62F, 0.0F};

    const unison::sim::PhysicsWorld world{settings};

    REQUIRE(world.gravity().y == -1.62F);
}

TEST_CASE("a physics world keeps stepping after another one is destroyed")
{
    unison::sim::PhysicsWorld world;

    {
        unison::sim::PhysicsWorld other;
        other.step(kTickSeconds);
    }

    world.step(kTickSeconds);

    REQUIRE(world.bodies().count() == 0U);
}

TEST_CASE("a world refuses a state buffer that ends before it should")
{
    const unison::test::FatalHandlerProbe probe;

    unison::sim::PhysicsWorld world;

    world.restoreState(std::span<const std::byte>{});

    REQUIRE(probe.failureCount() > 0U);
}
