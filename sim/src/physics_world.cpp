#include <unison/sim/physics_world.hpp>

#include <unison/core/body_id.hpp>
#include <unison/core/contract.hpp>
#include <unison/core/jolt_conversions.hpp>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/StateRecorder.h>

#include <algorithm>
#include <cstring>

namespace unison::sim
{

namespace
{

constexpr JPH::uint kBodyMutexCountForOneThread = 1;
constexpr int kCollisionStepsPerTick = 1;

JPH::BodyID bodyOf(const JPH::RayCastResult& hit)
{
    return hit.mBodyID;
}

JPH::BodyID bodyOf(const JPH::CollideShapeResult& hit)
{
    return hit.mBodyID2;
}

JPH::SubShapeID subShapeOf(const JPH::RayCastResult& hit)
{
    return hit.mSubShapeID2;
}

JPH::SubShapeID subShapeOf(const JPH::CollideShapeResult& hit)
{
    return hit.mSubShapeID2;
}

template <typename Hit>
bool byBodyThenSubShape(const Hit& first, const Hit& second)
{
    const std::uint32_t firstBody = bodyOf(first).GetIndexAndSequenceNumber();
    const std::uint32_t secondBody = bodyOf(second).GetIndexAndSequenceNumber();

    if (firstBody != secondBody)
    {
        return firstBody < secondBody;
    }

    return subShapeOf(first).GetValue() < subShapeOf(second).GetValue();
}

template <typename Hit>
bool nearestFirst(const Hit& first, const Hit& second)
{
    if (first.mFraction != second.mFraction)
    {
        return first.mFraction < second.mFraction;
    }

    return byBodyThenSubShape(first, second);
}

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

BodyMotion toBodyMotion(JPH::EMotionType motion)
{
    switch (motion)
    {
        case JPH::EMotionType::Dynamic:
            return BodyMotion::Dynamic;
        case JPH::EMotionType::Kinematic:
            return BodyMotion::Kinematic;
        case JPH::EMotionType::Static:
            break;
    }

    return BodyMotion::Static;
}

JPH::EActivation activationOf(BodyMotion motion)
{
    return motion == BodyMotion::Dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
}

}

PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings& settings)
    : runtime{}, scratchAllocator{settings.scratchBytes}, jobSystem{JPH::cMaxPhysicsJobs},
      characterTable{physicsSystem, scratchAllocator}
{
    physicsSystem.Init(kMaxBodies,
                       kBodyMutexCountForOneThread,
                       settings.maxBodyPairs,
                       settings.maxContactConstraints,
                       broadPhaseLayers,
                       objectVsBroadPhaseFilter,
                       objectPairFilter);

    physicsSystem.SetGravity(toJoltVector(settings.gravity));
    physicsSystem.SetContactListener(&contactCollector);
}

void PhysicsWorld::step(float dt)
{
    UNISON_ASSERT(dt > 0.0F);

    contactCollector.clear();

    const JPH::EPhysicsUpdateError error =
        physicsSystem.Update(dt, kCollisionStepsPerTick, &scratchAllocator, &jobSystem);

    contactCollector.sort();

    UNISON_VERIFY(error == JPH::EPhysicsUpdateError::None);
}

CharacterTable& PhysicsWorld::characters()
{
    return characterTable;
}

const CharacterTable& PhysicsWorld::characters() const
{
    return characterTable;
}

std::span<const Contact> PhysicsWorld::contacts() const
{
    return contactCollector.contacts();
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

    if (definition.mass > 0.0F)
    {
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = definition.mass;
    }

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

void PhysicsWorld::raycast(const Float3& from, const Float3& to, std::vector<PhysicsHit>& hits) const
{
    const JPH::RRayCast ray{toJoltVector(from), toJoltVector(to) - toJoltVector(from)};

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;

    physicsSystem.GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings{}, collector);

    std::sort(collector.mHits.begin(), collector.mHits.end(), nearestFirst<JPH::RayCastResult>);

    hits.clear();
    hits.reserve(collector.mHits.size());

    for (const JPH::RayCastResult& hit : collector.mHits)
    {
        hits.push_back(PhysicsHit{toBodyId(hit.mBodyID), hit.mFraction});
    }
}

void PhysicsWorld::overlapSphere(const Float3& centre, float radius, std::vector<BodyId>& bodies) const
{
    JPH::SphereShapeSettings sphere{radius};
    sphere.SetEmbedded();

    const JPH::Ref<JPH::Shape> shape = createdShape(sphere);
    const JPH::RVec3 at = toJoltVector(centre);

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    physicsSystem.GetNarrowPhaseQuery().CollideShape(
        shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(at), JPH::CollideShapeSettings{}, at, collector);

    std::sort(collector.mHits.begin(), collector.mHits.end(), byBodyThenSubShape<JPH::CollideShapeResult>);

    bodies.clear();
    bodies.reserve(collector.mHits.size());

    for (const JPH::CollideShapeResult& hit : collector.mHits)
    {
        bodies.push_back(toBodyId(hit.mBodyID2));
    }
}

void PhysicsWorld::sweepCapsule(
    const Float3& from, const Float3& to, float radius, float halfHeight, std::vector<PhysicsHit>& hits) const
{
    JPH::CapsuleShapeSettings capsule{halfHeight, radius};
    capsule.SetEmbedded();

    const JPH::Ref<JPH::Shape> shape = createdShape(capsule);
    const JPH::RVec3 at = toJoltVector(from);
    const JPH::RShapeCast sweep = JPH::RShapeCast::sFromWorldTransform(
        shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(at), toJoltVector(to) - at);

    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;

    physicsSystem.GetNarrowPhaseQuery().CastShape(sweep, JPH::ShapeCastSettings{}, at, collector);

    std::sort(collector.mHits.begin(), collector.mHits.end(), nearestFirst<JPH::ShapeCastResult>);

    hits.clear();
    hits.reserve(collector.mHits.size());

    for (const JPH::ShapeCastResult& hit : collector.mHits)
    {
        hits.push_back(PhysicsHit{toBodyId(hit.mBodyID2), hit.mFraction});
    }
}

void PhysicsWorld::applyProperties(BodyId id, const BodyDefinition& definition)
{
    UNISON_VERIFY(holdsBody(id));

    JPH::BodyInterface& bodies = physicsSystem.GetBodyInterface();
    const JPH::BodyID joltId = toJoltBodyId(id);
    const JPH::EMotionType motion = toJoltMotionType(definition.motion);

    bodies.SetFriction(joltId, definition.friction);
    bodies.SetRestitution(joltId, definition.restitution);
    bodies.SetObjectLayer(joltId, toObjectLayer(definition.layer));

    if (bodies.GetMotionType(joltId) != motion)
    {
        bodies.SetMotionType(joltId, motion, JPH::EActivation::DontActivate);
    }
}

float PhysicsWorld::frictionOf(BodyId id) const
{
    UNISON_ASSERT(holdsBody(id));

    return physicsSystem.GetBodyInterface().GetFriction(toJoltBodyId(id));
}

float PhysicsWorld::restitutionOf(BodyId id) const
{
    UNISON_ASSERT(holdsBody(id));

    return physicsSystem.GetBodyInterface().GetRestitution(toJoltBodyId(id));
}

float PhysicsWorld::massOf(BodyId id) const
{
    const JPH::BodyLockRead lock{physicsSystem.GetBodyLockInterface(), toJoltBodyId(id)};

    UNISON_VERIFY(lock.Succeeded());

    if (!lock.Succeeded() || !lock.GetBody().IsDynamic())
    {
        return 0.0F;
    }

    return 1.0F / lock.GetBody().GetMotionProperties()->GetInverseMass();
}

BodyMotion PhysicsWorld::motionOf(BodyId id) const
{
    UNISON_ASSERT(holdsBody(id));

    return toBodyMotion(physicsSystem.GetBodyInterface().GetMotionType(toJoltBodyId(id)));
}

void PhysicsWorld::collectBodies(std::vector<BodyId>& bodies) const
{
    JPH::BodyIDVector held;

    physicsSystem.GetBodies(held);

    bodies.clear();
    bodies.reserve(held.size());

    for (const JPH::BodyID& id : held)
    {
        bodies.push_back(toBodyId(id));
    }
}

void PhysicsWorld::saveState(std::vector<std::byte>& bytes) const
{
    bytes.clear();

    VectorStateRecorder recorder{bytes};

    physicsSystem.SaveState(recorder, JPH::EStateRecorderState::All);
    characterTable.saveState(recorder);

    UNISON_VERIFY(!recorder.IsFailed());
}

void PhysicsWorld::restoreState(std::span<const std::byte> bytes)
{
    std::vector<std::byte> recorded{bytes.begin(), bytes.end()};
    VectorStateRecorder recorder{recorded};

    contactCollector.clear();

    const bool restored = physicsSystem.RestoreState(recorder);

    characterTable.restoreState(recorder);

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
