#include <unison/sim/component_registry.hpp>

#include <support/fatal_handler_probe.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>

namespace
{

struct Position
{
    float x = 0.0F;
    float y = 0.0F;
};

struct Health
{
    std::int32_t points = 0;
};

constexpr std::string_view kOwnFile = "component_registry_test.cpp";
constexpr std::string_view kOtherFile = "somewhere_else.cpp";

unison::sim::ComponentInfo infoOf(std::string_view name, std::size_t size, std::size_t alignment)
{
    return unison::sim::ComponentInfo{name, size, alignment};
}

}

UNISON_COMPONENT(Position);

TEST_CASE("a component registry keeps the order components were added in")
{
    unison::sim::ComponentRegistry registry;

    registry.add(infoOf("Health", sizeof(Health), alignof(Health)), kOwnFile);
    registry.add(infoOf("Position", sizeof(Position), alignof(Position)), kOwnFile);

    const auto components = registry.components();

    REQUIRE(components.size() == 2U);
    REQUIRE(components[0].name == "Health");
    REQUIRE(components[1].name == "Position");
}

TEST_CASE("a component registry records the size and alignment of a component")
{
    unison::sim::ComponentRegistry registry;

    registry.add(infoOf("Position", sizeof(Position), alignof(Position)), kOwnFile);

    REQUIRE(registry.components()[0].size == sizeof(Position));
    REQUIRE(registry.components()[0].alignment == alignof(Position));
}

TEST_CASE("a component registry rejects a second component with the same name")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::ComponentRegistry registry;

    registry.add(infoOf("Position", sizeof(Position), alignof(Position)), kOwnFile);
    registry.add(infoOf("Position", sizeof(Position), alignof(Position)), kOwnFile);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a component registry rejects a component registered from another translation unit")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::ComponentRegistry registry;

    registry.add(infoOf("Position", sizeof(Position), alignof(Position)), kOwnFile);
    registry.add(infoOf("Health", sizeof(Health), alignof(Health)), kOtherFile);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("the component macro registers into the process-wide registry")
{
    const auto components = unison::sim::componentRegistry().components();
    bool found = false;

    for (const unison::sim::ComponentInfo& component : components)
    {
        found = found || component.name == "Position";
    }

    REQUIRE(found);
}
