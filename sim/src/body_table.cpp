#include <unison/sim/body_table.hpp>

#include <unison/core/contract.hpp>
#include <unison/core/jolt_conversions.hpp>
#include <unison/sim/jolt_layers.hpp>
#include <unison/sim/jolt_shapes.hpp>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>

namespace unison::sim
{

namespace
{

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

BodyTable::BodyTable(JPH::PhysicsSystem& system) : system{system}
{
    heldIds.reserve(kMaxBodies);
}

void BodyTable::create(BodyId id, const BodyDefinition& definition, const Transform& placement)
{
    UNISON_VERIFY(!holds(id));

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

    JPH::BodyInterface& bodies = system.GetBodyInterface();
    const JPH::Body* body = bodies.CreateBodyWithID(toJoltBodyId(id), settings);

    UNISON_VERIFY(body != nullptr);

    if (body == nullptr)
    {
        return;
    }

    bodies.AddBody(toJoltBodyId(id), activationOf(definition.motion));
}

void BodyTable::destroy(BodyId id)
{
    const bool held = holds(id);

    UNISON_VERIFY(held);

    if (!held)
    {
        return;
    }

    JPH::BodyInterface& bodies = system.GetBodyInterface();
    const JPH::BodyID joltId = toJoltBodyId(id);

    bodies.RemoveBody(joltId);
    bodies.DestroyBody(joltId);
}

bool BodyTable::holds(BodyId id) const
{
    const JPH::BodyLockRead lock{system.GetBodyLockInterface(), toJoltBodyId(id)};

    return lock.Succeeded();
}

Transform BodyTable::transformOf(BodyId id) const
{
    const JPH::BodyLockRead lock{system.GetBodyLockInterface(), toJoltBodyId(id)};

    UNISON_VERIFY(lock.Succeeded());

    if (!lock.Succeeded())
    {
        return Transform{};
    }

    return Transform{toFloat3(lock.GetBody().GetPosition()), toQuaternion(lock.GetBody().GetRotation())};
}

void BodyTable::applyProperties(BodyId id, const BodyDefinition& definition)
{
    UNISON_VERIFY(holds(id));

    JPH::BodyInterface& bodies = system.GetBodyInterface();
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

float BodyTable::frictionOf(BodyId id) const
{
    UNISON_ASSERT(holds(id));

    return system.GetBodyInterface().GetFriction(toJoltBodyId(id));
}

float BodyTable::restitutionOf(BodyId id) const
{
    UNISON_ASSERT(holds(id));

    return system.GetBodyInterface().GetRestitution(toJoltBodyId(id));
}

float BodyTable::massOf(BodyId id) const
{
    const JPH::BodyLockRead lock{system.GetBodyLockInterface(), toJoltBodyId(id)};

    UNISON_VERIFY(lock.Succeeded());

    if (!lock.Succeeded() || !lock.GetBody().IsDynamic())
    {
        return 0.0F;
    }

    return 1.0F / lock.GetBody().GetMotionProperties()->GetInverseMass();
}

BodyMotion BodyTable::motionOf(BodyId id) const
{
    UNISON_ASSERT(holds(id));

    return toBodyMotion(system.GetBodyInterface().GetMotionType(toJoltBodyId(id)));
}

void BodyTable::keepOnly(std::span<const BodyId, kMaxBodies> named)
{
    system.GetBodies(heldIds);

    for (const JPH::BodyID& joltId : heldIds)
    {
        const BodyId id = toBodyId(joltId);

        if (named[bodyIndexOf(id)] != id)
        {
            destroy(id);
        }
    }
}

std::uint32_t BodyTable::count() const
{
    return system.GetNumBodies();
}

}
