#include <unison/sim/physics_world.hpp>

#include <unison/core/body_id.hpp>
#include <unison/core/contract.hpp>
#include <unison/core/jolt_conversions.hpp>

namespace unison::sim
{

namespace
{

constexpr JPH::uint kBodyMutexCountForOneThread = 1;
constexpr int kCollisionStepsPerTick = 1;

}

PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings& settings)
    : runtime{}, scratchAllocator{settings.scratchBytes}, jobSystem{JPH::cMaxPhysicsJobs}
{
    physicsSystem.Init(kMaxBodies,
                       kBodyMutexCountForOneThread,
                       settings.maxBodyPairs,
                       settings.maxContactConstraints,
                       broadPhaseLayers,
                       objectVsBroadPhaseFilter,
                       objectPairFilter);

    physicsSystem.SetGravity(toJoltVector(settings.gravity));
}

void PhysicsWorld::step(float dt)
{
    UNISON_ASSERT(dt > 0.0F);

    const JPH::EPhysicsUpdateError error =
        physicsSystem.Update(dt, kCollisionStepsPerTick, &scratchAllocator, &jobSystem);

    UNISON_VERIFY(error == JPH::EPhysicsUpdateError::None);
}

std::uint32_t PhysicsWorld::bodyCount() const
{
    return physicsSystem.GetNumBodies();
}

Float3 PhysicsWorld::gravity() const
{
    return toFloat3(physicsSystem.GetGravity());
}

}
