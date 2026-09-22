#include <unison/sim/entity_lifecycle.hpp>

#include <catch2/catch_test_macros.hpp>

#include <entt/entity/registry.hpp>

namespace
{

constexpr std::uint32_t kFrame = 12;

}

TEST_CASE("creating an entity through the frame raises an event carrying it")
{
    unison::sim::Frame frame;
    frame.frameNumber = kFrame;

    const entt::entity entity = unison::sim::createEntity(frame);

    REQUIRE(frame.events.size() == 1U);
    REQUIRE(frame.events.payloadAt<unison::sim::EntityCreated>(0).entity == entity);
    REQUIRE(frame.events.keyAt(0).frame == kFrame);
}

TEST_CASE("an entity created through the frame lives in its registry")
{
    unison::sim::Frame frame;

    const entt::entity entity = unison::sim::createEntity(frame);

    REQUIRE(frame.registry.valid(entity));
}

TEST_CASE("destroying an entity through the frame raises an event carrying it")
{
    unison::sim::Frame frame;
    frame.frameNumber = kFrame;

    const entt::entity entity = unison::sim::createEntity(frame);
    frame.events.clear();

    unison::sim::destroyEntity(frame, entity);

    REQUIRE(frame.events.size() == 1U);
    REQUIRE(frame.events.payloadAt<unison::sim::EntityDestroyed>(0).entity == entity);
    REQUIRE(frame.events.keyAt(0).frame == kFrame);
}

TEST_CASE("an entity destroyed through the frame is gone from its registry")
{
    unison::sim::Frame frame;

    const entt::entity entity = unison::sim::createEntity(frame);
    unison::sim::destroyEntity(frame, entity);

    REQUIRE_FALSE(frame.registry.valid(entity));
}

TEST_CASE("the lifecycle events of one frame follow one another in order")
{
    unison::sim::Frame frame;

    const entt::entity first = unison::sim::createEntity(frame);
    const entt::entity second = unison::sim::createEntity(frame);
    unison::sim::destroyEntity(frame, first);

    REQUIRE(frame.events.size() == 3U);
    REQUIRE(frame.events.keyAt(0).ordinal == 0U);
    REQUIRE(frame.events.keyAt(1).ordinal == 1U);
    REQUIRE(frame.events.keyAt(2).ordinal == 0U);
    REQUIRE(frame.events.payloadAt<unison::sim::EntityCreated>(1).entity == second);
}
