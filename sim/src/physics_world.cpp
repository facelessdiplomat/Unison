#include <unison/sim/physics_world.hpp>

#include <unison/core/body_id.hpp>
#include <unison/core/contract.hpp>
#include <unison/core/jolt_conversions.hpp>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/StateRecorder.h>

#include <cstring>

namespace unison::sim
{

namespace
{

constexpr JPH::uint kBodyMutexCountForOneThread = 1;
constexpr int kCollisionStepsPerTick = 1;

JPH::Ref<JPH::Shape> createdShape(const JPH::ShapeSettings& settings)
{
    const JPH::ShapeSettings::ShapeResult result = settings.Create();

    UNISON_VERIFY(result.IsValid());

    return result.IsValid() ? result.Get() : JPH::Ref<JPH::Shape>{};
}

JPH::Ref<JPH::Shape> shapeOf(const BodyDefinition& definition)
{
    switch (definition.shape)
    {
        case BodyShape::Sphere:
        {
            JPH::SphereShapeSettings sphere{definition.radius};
            sphere.SetEmbedded();

            return createdShape(sphere);
        }
        case BodyShape::Capsule:
        {
            JPH::CapsuleShapeSettings capsule{definition.halfHeight, definition.radius};
            capsule.SetEmbedded();

            return createdShape(capsule);
        }
        case BodyShape::Box:
            break;
    }

    JPH::BoxShapeSettings box{toJoltVector(definition.halfExtents)};
    box.SetEmbedded();

    return createdShape(box);
}

JPH::EMotionType toJoltMotionType(BodyMotion motion)
{
    switch (motion)
    {
        case BodyMotion::Dynamic:
            return JPH::EMotionType::Dynamic;
        case BodyMotion::Kinematic:
            return JPH::EMotionType::Kinematic;
        case BodyMotion::Static:
            break;
    }

    return JPH::EMotionType::Static;
}

class VectorStateRecorder final : public JPH::StateRecorder
{
public:
    explicit VectorStateRecorder(std::vector<std::byte>& bytes) : recorded{bytes}
    {
    }

    void WriteBytes(const void* data, std::size_t count) override
    {
        const auto* first = static_cast<const std::byte*>(data);

        recorded.insert(recorded.end(), first, first + count);
    }

    void ReadBytes(void* data, std::size_t count) override
    {
        if (readPosition + count > recorded.size())
        {
            std::memset(data, 0, count);
            readPast = true;

            return;
        }

        std::memcpy(data, recorded.data() + readPosition, count);
        readPosition += count;
    }

    [[nodiscard]] bool IsEOF() const override
    {
        return readPast;
    }

    [[nodiscard]] bool IsFailed() const override
    {
        return readPast;
    }

private:
    std::vector<std::byte>& recorded;
    std::size_t readPosition = 0;
    bool readPast = false;
};

JPH::EActivation activationOf(BodyMotion motion)
{
    return motion == BodyMotion::Dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
}

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

void PhysicsWorld::createBody(BodyId id, const BodyDefinition& definition, const Transform& placement)
{
    UNISON_VERIFY(!holdsBody(id));

    const JPH::Ref<JPH::Shape> shape = shapeOf(definition);

    JPH::BodyCreationSettings settings{shape,
                                       toJoltVector(placement.position),
                                       toJoltQuaternion(placement.rotation),
                                       toJoltMotionType(definition.motion),
                                       toObjectLayer(definition.layer)};

    settings.mFriction = definition.friction;
    settings.mRestitution = definition.restitution;

    JPH::BodyInterface& bodies = physicsSystem.GetBodyInterface();
    const JPH::Body* body = bodies.CreateBodyWithID(toJoltBodyId(id), settings);

    UNISON_VERIFY(body != nullptr);

    if (body == nullptr)
    {
        return;
    }

    bodies.AddBody(toJoltBodyId(id), activationOf(definition.motion));
}

void PhysicsWorld::destroyBody(BodyId id)
{
    const bool held = holdsBody(id);

    UNISON_VERIFY(held);

    if (!held)
    {
        return;
    }

    JPH::BodyInterface& bodies = physicsSystem.GetBodyInterface();
    const JPH::BodyID joltId = toJoltBodyId(id);

    bodies.RemoveBody(joltId);
    bodies.DestroyBody(joltId);
}

bool PhysicsWorld::holdsBody(BodyId id) const
{
    const JPH::BodyLockRead lock{physicsSystem.GetBodyLockInterface(), toJoltBodyId(id)};

    return lock.Succeeded();
}

Transform PhysicsWorld::transformOf(BodyId id) const
{
    const JPH::BodyLockRead lock{physicsSystem.GetBodyLockInterface(), toJoltBodyId(id)};

    UNISON_VERIFY(lock.Succeeded());

    if (!lock.Succeeded())
    {
        return Transform{};
    }

    return Transform{toFloat3(lock.GetBody().GetPosition()), toQuaternion(lock.GetBody().GetRotation())};
}

void PhysicsWorld::saveState(std::vector<std::byte>& bytes) const
{
    bytes.clear();

    VectorStateRecorder recorder{bytes};

    physicsSystem.SaveState(recorder, JPH::EStateRecorderState::All);

    UNISON_VERIFY(!recorder.IsFailed());
}

void PhysicsWorld::restoreState(std::span<const std::byte> bytes)
{
    std::vector<std::byte> recorded{bytes.begin(), bytes.end()};
    VectorStateRecorder recorder{recorded};

    const bool restored = physicsSystem.RestoreState(recorder);

    UNISON_VERIFY(restored);
    UNISON_VERIFY(!recorder.IsFailed());
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
