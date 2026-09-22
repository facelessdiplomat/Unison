#include <arena/character_move.hpp>

#include <arena/arena_input.hpp>
#include <arena/components.hpp>

#include <unison/sim/character.hpp>

#include <entt/entity/registry.hpp>

namespace arena
{

CharacterMove::CharacterMove(const PlayerStats& stats) : stats{stats}
{
}

void CharacterMove::update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs)
{
    for (const auto [entity, player, state, character] :
         frame.registry.view<const PlayerSlot, CharacterState, unison::sim::CharacterController>().each())
    {
        const ArenaInput input = inputs.get<ArenaInput>(player.slot);
        const bool standing = character.ground == unison::sim::GroundState::OnGround && state.verticalSpeed <= 0.0F;

        if (standing)
        {
            state.verticalSpeed = isHeld(input, Button::Jump) ? stats.jumpSpeed : 0.0F;
        }

        state.verticalSpeed += stats.gravity * frame.dt;

        character.velocity = unison::Float3{state.desiredVelocity.x, state.verticalSpeed, state.desiredVelocity.z};
    }
}

std::string_view CharacterMove::name() const
{
    return "CharacterMove";
}

}
