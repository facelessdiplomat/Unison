#include <unison/sim/frame.hpp>

#include <catch2/catch_test_macros.hpp>

#include <entt/entity/registry.hpp>

#include <cstdint>

namespace
{

struct Marker
{
    std::uint32_t value = 0;
};

}

TEST_CASE("a default frame is at zero and holds nothing")
{
    const unison::sim::Frame frame;

    REQUIRE(frame.frameNumber == 0U);
    REQUIRE(frame.dt == 0.0F);
    REQUIRE(frame.registry.view<entt::entity>().size() == 0U);
}

TEST_CASE("a default frame starts its match in warmup")
{
    const unison::sim::Frame frame;

    REQUIRE(frame.globals.matchPhase == unison::sim::MatchPhase::Warmup);
}

TEST_CASE("a default frame carries a seeded generator")
{
    unison::sim::Frame first;
    unison::sim::Frame second;

    REQUIRE(first.globals.rng.nextUint32() == second.globals.rng.nextUint32());
}

TEST_CASE("a frame holds the entities created in it")
{
    unison::sim::Frame frame;
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<Marker>(entity, 7U);

    REQUIRE(frame.registry.view<Marker>().size() == 1U);
    REQUIRE(frame.registry.get<Marker>(entity).value == 7U);
}
