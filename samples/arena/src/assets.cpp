#include <arena/assets.hpp>

#include <unison/sim/body_definition.hpp>

namespace arena
{

namespace
{

unison::sim::BodyDefinition staticBox(const unison::Float3& halfExtents, float friction)
{
    unison::sim::BodyDefinition definition;
    definition.halfExtents = halfExtents;
    definition.shape = unison::sim::BodyShape::Box;
    definition.motion = unison::sim::BodyMotion::Static;
    definition.layer = unison::sim::PhysicsLayer::Static;
    definition.friction = friction;

    return definition;
}

unison::sim::BodyDefinition crate()
{
    unison::sim::BodyDefinition definition;
    definition.halfExtents = unison::Float3{0.5F, 0.5F, 0.5F};
    definition.shape = unison::sim::BodyShape::Box;
    definition.motion = unison::sim::BodyMotion::Dynamic;
    definition.layer = unison::sim::PhysicsLayer::Moving;
    definition.friction = 0.4F;
    definition.restitution = 0.1F;

    return definition;
}

SpawnPoints spawnPoints()
{
    SpawnPoints points;
    points.positions = {unison::Float3{-8.0F, 1.0F, -8.0F},
                        unison::Float3{8.0F, 1.0F, -8.0F},
                        unison::Float3{8.0F, 1.0F, 8.0F},
                        unison::Float3{-8.0F, 1.0F, 8.0F}};
    points.yaws = {0.7853982F, 2.3561945F, 3.9269907F, 5.4977871F};

    return points;
}

}

void defineArena(unison::sim::AssetRegistry& assets)
{
    assets.add<unison::sim::BodyDefinition>(kFloor, staticBox(unison::Float3{12.0F, 0.5F, 12.0F}, 0.6F));
    assets.add<unison::sim::BodyDefinition>(kWall, staticBox(unison::Float3{12.0F, 2.0F, 0.5F}, 0.4F));
    assets.add<unison::sim::BodyDefinition>(kRamp, staticBox(unison::Float3{3.0F, 0.25F, 2.0F}, 0.5F));
    assets.add<unison::sim::BodyDefinition>(kCrate, crate());
    assets.add<SpawnPoints>(kSpawnPoints, spawnPoints());
    assets.add<PlayerStats>(kPlayerStats, PlayerStats{});
    assets.add<ProjectileStats>(kProjectileStats, ProjectileStats{});

    assets.freeze();
}

}
