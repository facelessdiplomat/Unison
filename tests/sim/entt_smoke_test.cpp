#include <catch2/catch_test_macros.hpp>
#include <entt/entity/registry.hpp>

namespace
{

struct Position
{
    float x;
    float y;
};

}

TEST_CASE("a view finds an emplaced component")
{
    entt::registry registry;
    const entt::entity entity = registry.create();

    registry.emplace<Position>(entity, 1.0f, 2.0f);

    const auto view = registry.view<Position>();

    REQUIRE(view.size() == 1U);
    REQUIRE(view.contains(entity));
    REQUIRE(view.get<Position>(entity).x == 1.0f);
}
