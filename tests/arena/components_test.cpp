#include <arena/components.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/character.hpp>
#include <unison/sim/component_registry.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

#include <string_view>
#include <type_traits>

namespace
{

bool isRegistered(std::string_view name)
{
    for (const unison::sim::ComponentInfo& component : unison::sim::componentRegistry().components())
    {
        if (component.name == name)
        {
            return true;
        }
    }

    return false;
}

}

TEST_CASE("every component of the arena is plain data")
{
    STATIC_REQUIRE(std::is_trivially_copyable_v<arena::PlayerSlot>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<arena::CharacterState>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<arena::Health>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<arena::Weapon>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<arena::Projectile>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<arena::Lifetime>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<arena::RespawnTimer>);
}

TEST_CASE("every component of the arena carries no padding")
{
    STATIC_REQUIRE(unison::sim::PaddingFree<arena::PlayerSlot>);
    STATIC_REQUIRE(unison::sim::PaddingFree<arena::CharacterState>);
    STATIC_REQUIRE(unison::sim::PaddingFree<arena::Health>);
    STATIC_REQUIRE(unison::sim::PaddingFree<arena::Weapon>);
    STATIC_REQUIRE(unison::sim::PaddingFree<arena::Projectile>);
    STATIC_REQUIRE(unison::sim::PaddingFree<arena::Lifetime>);
    STATIC_REQUIRE(unison::sim::PaddingFree<arena::RespawnTimer>);
}

TEST_CASE("the arena registers what a snapshot has to carry")
{
    REQUIRE(isRegistered("Transform"));
    REQUIRE(isRegistered("BodyDefinition"));
    REQUIRE(isRegistered("PhysicsBody"));
    REQUIRE(isRegistered("CharacterController"));
    REQUIRE(isRegistered("PlayerSlot"));
    REQUIRE(isRegistered("CharacterState"));
    REQUIRE(isRegistered("Health"));
    REQUIRE(isRegistered("Weapon"));
    REQUIRE(isRegistered("Projectile"));
    REQUIRE(isRegistered("Lifetime"));
    REQUIRE(isRegistered("RespawnTimer"));
    REQUIRE(isRegistered("Score"));
    REQUIRE(isRegistered("Killed"));
}

TEST_CASE("the components of the arena are registered from one place and in one order")
{
    const std::span<const unison::sim::ComponentInfo> components = unison::sim::componentRegistry().components();

    REQUIRE(components.size() == 13U);
    REQUIRE(components[0].name == "Transform");
    REQUIRE(components[12].name == "Killed");
}
