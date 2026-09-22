#include <arena/apply_input.hpp>

#include <arena/components.hpp>
#include <arena/facing.hpp>

#include <unison/core/math.hpp>

#include <entt/entity/registry.hpp>

namespace arena
{

namespace
{

unison::Float3 askedVelocity(const ArenaInput& input, float yaw, float speed)
{
    const float forward = axisOf(input.moveY);
    const float strafe = axisOf(input.moveX);
    const float asked = unison::math::sqrt(forward * forward + strafe * strafe);
    const float scale = (asked > 1.0F ? 1.0F / asked : 1.0F) * speed;

    const unison::Float3 ahead = facingOf(yaw);
    const unison::Float3 right = rightOf(yaw);

    return unison::Float3{
        (forward * ahead.x + strafe * right.x) * scale, 0.0F, (forward * ahead.z + strafe * right.z) * scale};
}

}

ApplyInput::ApplyInput(const PlayerStats& stats) : stats{stats}
{
}

void ApplyInput::update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs)
{
    for (const auto [entity, player, state] : frame.registry.view<const PlayerSlot, CharacterState>().each())
    {
        const ArenaInput input = inputs.get<ArenaInput>(player.slot);
        const float yaw = yawOf(input);

        state.yaw = yaw;
        state.desiredVelocity = askedVelocity(input, yaw, stats.moveSpeed);
    }
}

std::string_view ApplyInput::name() const
{
    return "ApplyInput";
}

}
