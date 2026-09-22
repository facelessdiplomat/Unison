#include <unison/sim/registry_clone.hpp>

#include <support/test_components.hpp>

#include <catch2/catch_test_macros.hpp>

#include <entt/entity/registry.hpp>

#include <cstdint>
#include <vector>

namespace
{

constexpr int kOperationCount = 100;

void churn(entt::registry& registry)
{
    std::vector<entt::entity> living;

    for (int step = 0; step < kOperationCount; ++step)
    {
        if (step % 3 == 2 && !living.empty())
        {
            const std::size_t victim = static_cast<std::size_t>(step) % living.size();

            registry.destroy(living[victim]);
            living.erase(living.begin() + static_cast<std::ptrdiff_t>(victim));
        }
        else
        {
            const entt::entity entity = registry.create();

            registry.emplace<unison::test::Position>(entity, static_cast<float>(step), 0.0F);
            living.push_back(entity);
        }
    }
}

}

TEST_CASE("a clone hands out the same identifiers as its source from then on")
{
    entt::registry source;
    churn(source);

    entt::registry clone;
    unison::sim::cloneRegistry(source, clone);

    for (int step = 0; step < 20; ++step)
    {
        REQUIRE(clone.create() == source.create());
    }
}

TEST_CASE("a clone holds the same living entities as its source")
{
    entt::registry source;
    churn(source);

    entt::registry clone;
    unison::sim::cloneRegistry(source, clone);

    const auto sourceView = source.view<unison::test::Position>();
    const auto cloneView = clone.view<unison::test::Position>();

    REQUIRE(cloneView.size() == sourceView.size());

    for (const entt::entity entity : sourceView)
    {
        REQUIRE(clone.valid(entity));
        REQUIRE(clone.get<unison::test::Position>(entity).x == source.get<unison::test::Position>(entity).x);
    }
}

TEST_CASE("a clone iterates its pool in the same order as its source")
{
    entt::registry source;
    churn(source);

    entt::registry clone;
    unison::sim::cloneRegistry(source, clone);

    std::vector<entt::entity> fromSource;
    std::vector<entt::entity> fromClone;

    for (const entt::entity entity : source.view<unison::test::Position>())
    {
        fromSource.push_back(entity);
    }

    for (const entt::entity entity : clone.view<unison::test::Position>())
    {
        fromClone.push_back(entity);
    }

    REQUIRE(fromClone == fromSource);
}
