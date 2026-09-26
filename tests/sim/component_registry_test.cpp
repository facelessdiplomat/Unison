#include <unison/sim/component_registry.hpp>

#include <support/fatal_handler_probe.hpp>
#include <support/test_components.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace
{

constexpr std::string_view kOwnFile = "component_registry_test.cpp";
constexpr std::string_view kOtherFile = "somewhere_else.cpp";

unison::sim::ComponentInfo infoOf(std::string_view name, std::size_t size, std::size_t alignment)
{
    return unison::sim::ComponentInfo{name, size, alignment};
}

}

TEST_CASE("a component registry keeps the order components were added in")
{
    unison::sim::ComponentRegistry registry;

    registry.add(infoOf("Health", sizeof(unison::test::Health), alignof(unison::test::Health)), kOwnFile);
    registry.add(infoOf("Position", sizeof(unison::test::Position), alignof(unison::test::Position)), kOwnFile);

    const auto components = registry.components();

    REQUIRE(components.size() == 2U);
    REQUIRE(components[0].name == "Health");
    REQUIRE(components[1].name == "Position");
}

TEST_CASE("a component registry records the size and alignment of a component")
{
    unison::sim::ComponentRegistry registry;

    registry.add(infoOf("Position", sizeof(unison::test::Position), alignof(unison::test::Position)), kOwnFile);

    REQUIRE(registry.components()[0].size == sizeof(unison::test::Position));
    REQUIRE(registry.components()[0].alignment == alignof(unison::test::Position));
}

TEST_CASE("a component registry rejects a second component with the same name")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::ComponentRegistry registry;

    registry.add(infoOf("Position", sizeof(unison::test::Position), alignof(unison::test::Position)), kOwnFile);
    registry.add(infoOf("Position", sizeof(unison::test::Position), alignof(unison::test::Position)), kOwnFile);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a component registry rejects a component registered from another translation unit")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::ComponentRegistry registry;

    registry.add(infoOf("Position", sizeof(unison::test::Position), alignof(unison::test::Position)), kOwnFile);
    registry.add(infoOf("Health", sizeof(unison::test::Health), alignof(unison::test::Health)), kOtherFile);

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

TEST_CASE("the registry answers which components the game registered")
{
    REQUIRE(unison::sim::isComponentRegistered("Transform"));
    REQUIRE(unison::sim::isComponentRegistered("PhysicsBody"));
    REQUIRE_FALSE(unison::sim::isComponentRegistered("AComponentNobodyRegistered"));
}

TEST_CASE("fields named by the macro cover their component byte for byte in declaration order")
{
    const auto isPosition = [](const unison::sim::ComponentInfo& component)
    {
        return component.name == "Position";
    };
    const auto components = unison::sim::componentRegistry().components();
    const auto position = std::ranges::find_if(components, isPosition);
    REQUIRE(position != components.end());

    const auto fields = position->fields;

    REQUIRE(fields.size() == 2U);
    REQUIRE(fields[0].name == "x");
    REQUIRE(fields[0].offset == 0U);
    REQUIRE(fields[0].size == sizeof(float));
    REQUIRE(fields[1].name == "y");
    REQUIRE(fields[1].offset == sizeof(float));
    REQUIRE(fields[1].size == sizeof(float));
}

TEST_CASE("field names are counted by the commas between them")
{
    STATIC_REQUIRE(unison::sim::countFieldNames("points") == 1U);
    STATIC_REQUIRE(unison::sim::countFieldNames("x, y") == 2U);
    STATIC_REQUIRE(unison::sim::countFieldNames("") == 0U);
}

TEST_CASE("naming the fields of a component registered in another file breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::ComponentRegistry registry;
    registry.add(infoOf("Health", sizeof(unison::test::Health), alignof(unison::test::Health)), kOwnFile);
    const std::array<unison::sim::FieldInfo, 1> fields{unison::sim::FieldInfo{"points", 0, sizeof(std::int32_t)}};

    registry.nameFields("Health", fields, kOtherFile);

    REQUIRE(probe.failureCount() == 1U);
}
