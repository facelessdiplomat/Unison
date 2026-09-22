#include <arena/weapons.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/arena_input.hpp>
#include <arena/assets.hpp>
#include <arena/components.hpp>
#include <arena/events.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

entt::entity armPlayer(unison::sim::Frame& frame, std::uint8_t slot, float yaw)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::sim::Transform>(entity, unison::Float3{0.0F, 0.5F, 0.0F}, unison::Quaternion{});
    frame.registry.emplace<arena::PlayerSlot>(entity, slot);
    frame.registry.emplace<arena::CharacterState>(entity, unison::Float3{}, yaw, 0.0F);
    frame.registry.emplace<arena::Weapon>(entity);

    return entity;
}

unison::sim::FrameInputs trigger(std::uint8_t slot, bool fire)
{
    arena::ArenaInput input;

    if (fire)
    {
        input.buttons = static_cast<std::uint16_t>(arena::Button::Fire);
    }

    unison::sim::FrameInputs inputs;
    inputs.set(slot, input, unison::sim::InputFlags::Present);

    return inputs;
}

std::size_t shotsInFlight(const unison::sim::Frame& frame)
{
    return frame.registry.view<const arena::Projectile>().size();
}

std::size_t firedEventsIn(const unison::sim::Frame& frame)
{
    std::size_t fired = 0;

    for (std::size_t index = 0; index < frame.events.size(); ++index)
    {
        if (frame.events.keyAt(index).typeId == unison::sim::EventTraits<arena::Fired>::typeId)
        {
            ++fired;
        }
    }

    return fired;
}

}

TEST_CASE("pulling the trigger sends a shot on its way")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    arena::Weapons weapons{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(weapons);

    unison::sim::Frame frame;
    const entt::entity player = armPlayer(frame, 0U, 0.0F);

    unison::sim::advanceFrame(frame, pipeline, trigger(0U, true));

    REQUIRE(shotsInFlight(frame) == 1U);
    REQUIRE(firedEventsIn(frame) == 1U);

    const auto view = frame.registry.view<const arena::Projectile>();
    const entt::entity shot = view.front();

    REQUIRE(frame.registry.get<arena::Projectile>(shot).firedBy == player);
    REQUIRE(frame.registry.get<arena::Projectile>(shot).velocity.z > 1.0F);
    REQUIRE(frame.registry.get<unison::sim::Transform>(shot).position.y > 1.0F);
}

TEST_CASE("a shot leaves along the way its player faces")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    arena::Weapons weapons{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(weapons);

    unison::sim::Frame frame;
    armPlayer(frame, 0U, 1.5707964F);

    unison::sim::advanceFrame(frame, pipeline, trigger(0U, true));

    const entt::entity shot = frame.registry.view<const arena::Projectile>().front();
    const unison::Float3 velocity = frame.registry.get<arena::Projectile>(shot).velocity;

    REQUIRE(velocity.x > 1.0F);
    REQUIRE(velocity.z < 1.0F);
    REQUIRE(velocity.z > -1.0F);
}

TEST_CASE("a weapon holds its fire until it has cooled down")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::ProjectileStats& stats = assets.get<arena::ProjectileStats>(arena::kProjectileStats);

    arena::Weapons weapons{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(weapons);

    unison::sim::Frame frame;
    armPlayer(frame, 0U, 0.0F);

    for (std::uint32_t tick = 0; tick < 2U * stats.cooldownFrames; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, trigger(0U, true));
    }

    REQUIRE(shotsInFlight(frame) == 2U);
}

TEST_CASE("a weapon fires again once its cooldown has run out")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    const arena::ProjectileStats& stats = assets.get<arena::ProjectileStats>(arena::kProjectileStats);

    arena::Weapons weapons{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(weapons);

    unison::sim::Frame frame;
    armPlayer(frame, 0U, 0.0F);

    unison::sim::advanceFrame(frame, pipeline, trigger(0U, true));

    for (std::uint32_t tick = 0; tick < stats.cooldownFrames - 1U; ++tick)
    {
        unison::sim::advanceFrame(frame, pipeline, trigger(0U, false));
    }

    REQUIRE(shotsInFlight(frame) == 1U);

    unison::sim::advanceFrame(frame, pipeline, trigger(0U, true));

    REQUIRE(shotsInFlight(frame) == 2U);
}

TEST_CASE("two players firing at once get an ordinal each")
{
    unison::sim::AssetRegistry assets;
    arena::defineArena(assets);

    arena::Weapons weapons{assets};
    unison::sim::SystemPipeline pipeline;
    pipeline.add(weapons);

    unison::sim::Frame frame;
    armPlayer(frame, 0U, 0.0F);
    armPlayer(frame, 1U, 0.0F);

    arena::ArenaInput input;
    input.buttons = static_cast<std::uint16_t>(arena::Button::Fire);

    unison::sim::FrameInputs inputs;
    inputs.set(0U, input, unison::sim::InputFlags::Present);
    inputs.set(1U, input, unison::sim::InputFlags::Present);

    unison::sim::advanceFrame(frame, pipeline, inputs);

    REQUIRE(shotsInFlight(frame) == 2U);
    REQUIRE(firedEventsIn(frame) == 2U);

    std::uint32_t ordinals = 0;

    for (std::size_t index = 0; index < frame.events.size(); ++index)
    {
        if (frame.events.keyAt(index).typeId == unison::sim::EventTraits<arena::Fired>::typeId)
        {
            ordinals |= 1U << frame.events.keyAt(index).ordinal;
        }
    }

    REQUIRE(ordinals == 0b11U);
}
