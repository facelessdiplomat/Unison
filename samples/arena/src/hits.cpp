#include <arena/hits.hpp>

#include <arena/assets.hpp>
#include <arena/components.hpp>
#include <arena/events.hpp>
#include <arena/shot_path.hpp>

#include <unison/sim/character.hpp>
#include <unison/sim/entity_lifecycle.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

namespace arena
{

namespace
{

unison::Float3 along(const unison::Float3& from, const unison::Float3& to, float fraction)
{
    return unison::Float3{
        from.x + (to.x - from.x) * fraction, from.y + (to.y - from.y) * fraction, from.z + (to.z - from.z) * fraction};
}

unison::Float3 flownTo(const unison::sim::Transform& stance, const Projectile& shot, float dt)
{
    return unison::Float3{stance.position.x + shot.velocity.x * dt,
                          stance.position.y + shot.velocity.y * dt,
                          stance.position.z + shot.velocity.z * dt};
}

struct PlayerReached
{
    entt::entity player = entt::null;
    float fraction = 1.0F;
};

PlayerReached reachedPlayer(const unison::sim::Frame& frame,
                            const unison::Float3& from,
                            const unison::Float3& to,
                            float shotRadius,
                            entt::entity firedBy)
{
    PlayerReached nearest;

    for (const auto [entity, stance, capsule, health] :
         frame.registry.view<const unison::sim::Transform, const unison::sim::CharacterController, const Health>()
             .each())
    {
        if (entity == firedBy)
        {
            continue;
        }

        const unison::Float3 bottom{stance.position.x, stance.position.y + capsule.radius, stance.position.z};
        const unison::Float3 top{bottom.x, bottom.y + 2.0F * capsule.halfHeight, bottom.z};
        const ShotApproach approach = sweepPastCapsule(from, to, shotRadius, bottom, top, capsule.radius);

        if (approach.touched && (nearest.player == entt::null || approach.fraction < nearest.fraction))
        {
            nearest = PlayerReached{entity, approach.fraction};
        }
    }

    return nearest;
}

void strike(unison::sim::Frame& frame,
            entt::entity shotEntity,
            const Projectile& shot,
            entt::entity target,
            const unison::Float3& at)
{
    Health& health = frame.registry.get<Health>(target);

    health.points -= shot.damage;
    health.lastHitBy = shot.firedBy;

    frame.events.raise(frame.frameNumber, Hit{shotEntity, target, shot.firedBy, at});
}

}

Hits::Hits(const unison::sim::AssetRegistry& assets) : assets{assets}
{
}

void Hits::update(unison::sim::Frame& frame, const unison::sim::FrameInputs&)
{
    const ProjectileStats& stats = assets.get<ProjectileStats>(kProjectileStats);

    for (const auto [entity, shot, stance] : frame.registry.view<const Projectile, unison::sim::Transform>().each())
    {
        const unison::Float3 from = stance.position;
        const unison::Float3 to = flownTo(stance, shot, frame.dt);

        const PlayerReached player = reachedPlayer(frame, from, to, stats.radius, shot.firedBy);

        if (player.player != entt::null)
        {
            strike(frame, entity, shot, player.player, along(from, to, player.fraction));
            unison::sim::destroyEntity(frame, entity);

            continue;
        }

        if (frame.physics.queries().raycastNearest(from, to).has_value())
        {
            unison::sim::destroyEntity(frame, entity);

            continue;
        }

        stance.position = to;
    }
}

std::string_view Hits::name() const
{
    return "Hits";
}

}
