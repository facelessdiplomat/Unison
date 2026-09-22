#include <arena/weapons.hpp>

#include <arena/arena_input.hpp>
#include <arena/assets.hpp>
#include <arena/components.hpp>
#include <arena/events.hpp>
#include <arena/facing.hpp>

#include <unison/core/contract.hpp>
#include <unison/core/fixed_vector.hpp>
#include <unison/sim/entity_lifecycle.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

namespace arena
{

namespace
{

struct PendingShot
{
    entt::entity player = entt::null;
    unison::Float3 from{};
    unison::Float3 direction{};
};

constexpr float kMuzzleClearance = 0.05F;

unison::Float3 muzzleOf(const unison::sim::Transform& stance,
                        const unison::Float3& direction,
                        const PlayerStats& player,
                        const ProjectileStats& shot)
{
    const float clearance = player.capsuleRadius + shot.radius + kMuzzleClearance;

    return unison::Float3{stance.position.x + direction.x * clearance,
                          stance.position.y + player.eyeHeight,
                          stance.position.z + direction.z * clearance};
}

}

Weapons::Weapons(const unison::sim::AssetRegistry& assets) : assets{assets}
{
}

void Weapons::update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs)
{
    const PlayerStats& player = assets.get<PlayerStats>(kPlayerStats);
    const ProjectileStats& shot = assets.get<ProjectileStats>(kProjectileStats);

    unison::FixedVector<PendingShot, unison::sim::kMaxSlots> pending;

    for (const auto [entity, slot, state, weapon, stance] :
         frame.registry.view<const PlayerSlot, const CharacterState, Weapon, const unison::sim::Transform>().each())
    {
        if (weapon.framesUntilReady > 0U)
        {
            --weapon.framesUntilReady;
        }

        const ArenaInput input = inputs.get<ArenaInput>(slot.slot);

        if (!isHeld(input, Button::Fire) || weapon.framesUntilReady > 0U)
        {
            continue;
        }

        UNISON_VERIFY(!pending.isFull());

        if (pending.isFull())
        {
            continue;
        }

        weapon.framesUntilReady = shot.cooldownFrames;

        const unison::Float3 direction = facingOf(state.yaw);

        pending.pushBack(PendingShot{entity, muzzleOf(stance, direction, player, shot), direction});
    }

    for (const PendingShot& fired : pending)
    {
        const entt::entity flying = unison::sim::createEntity(frame);

        frame.registry.emplace<unison::sim::Transform>(flying, fired.from, unison::Quaternion{});
        frame.registry.emplace<Projectile>(flying,
                                           fired.player,
                                           shot.damage,
                                           unison::Float3{fired.direction.x * shot.speed,
                                                          fired.direction.y * shot.speed,
                                                          fired.direction.z * shot.speed});
        frame.registry.emplace<Lifetime>(flying, shot.lifetimeFrames);

        frame.events.raise(frame.frameNumber, Fired{fired.player, flying});
    }
}

std::string_view Weapons::name() const
{
    return "Weapons";
}

}
