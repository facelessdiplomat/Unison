#include <arena/lifetimes.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/components.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/entity_lifecycle.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstdint>

namespace
{

constexpr std::uint32_t kStay = 5;

entt::entity somethingThatWillGo(unison::sim::Frame& frame, std::uint32_t frames)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity);
    frame.registry.emplace<arena::Lifetime>(entity, frames);

    return entity;
}

void run(unison::sim::Frame& frame, const unison::sim::SystemPipeline& pipeline, std::uint32_t ticks)
{
    for (std::uint32_t tick = 0; tick < ticks; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});
    }
}

}

TEST_CASE("what was given a stay is still there the tick before it runs out")
{
    arena::Lifetimes lifetimes;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(lifetimes);

    unison::sim::Frame frame;
    const entt::entity going = somethingThatWillGo(frame, kStay);

    run(frame, pipeline, kStay - 1U);

    REQUIRE(frame.registry.valid(going));
    REQUIRE(frame.registry.get<arena::Lifetime>(going).framesLeft == 1U);
}

TEST_CASE("what was given a stay is gone at the frame it runs out")
{
    arena::Lifetimes lifetimes;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(lifetimes);

    unison::sim::Frame frame;
    const entt::entity going = somethingThatWillGo(frame, kStay);

    run(frame, pipeline, kStay);

    REQUIRE(frame.frameNumber == kStay);
    REQUIRE_FALSE(frame.registry.valid(going));
}

TEST_CASE("something going tells the view that it went")
{
    arena::Lifetimes lifetimes;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(lifetimes);

    unison::sim::Frame frame;
    const entt::entity going = somethingThatWillGo(frame, 1U);

    unison::sim::advanceFrame(frame, pipeline, unison::sim::FrameInputs{});

    REQUIRE(frame.events.size() == 1U);
    REQUIRE(frame.events.payloadAt<unison::sim::EntityDestroyed>(0).entity == going);
}

TEST_CASE("what has no stay to run out is left alone")
{
    arena::Lifetimes lifetimes;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(lifetimes);

    unison::sim::Frame frame;
    const entt::entity staying = frame.registry.create();
    frame.registry.emplace<unison::sim::Transform>(staying);

    run(frame, pipeline, 100U);

    REQUIRE(frame.registry.valid(staying));
}
