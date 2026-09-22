#include <unison/sim/jolt_runtime.hpp>

#include <catch2/catch_test_macros.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>

TEST_CASE("jolt is registered while a runtime scope is open")
{
    const unison::sim::JoltRuntime runtime;

    REQUIRE(JPH::Factory::sInstance != nullptr);
}

TEST_CASE("jolt is unregistered once the last runtime scope closes")
{
    {
        const unison::sim::JoltRuntime runtime;
    }

    REQUIRE(JPH::Factory::sInstance == nullptr);
}

TEST_CASE("jolt stays registered while an outer runtime scope is open")
{
    const unison::sim::JoltRuntime outer;

    {
        const unison::sim::JoltRuntime inner;
    }

    REQUIRE(JPH::Factory::sInstance != nullptr);
}
