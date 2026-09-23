#include <unison/sim/physics_queries.hpp>

#include <unison/core/jolt_conversions.hpp>
#include <unison/sim/jolt_shapes.hpp>

#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

#include <algorithm>
#include <cstdint>

namespace unison::sim
{

namespace
{

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

}

PhysicsQueries::PhysicsQueries(const JPH::PhysicsSystem& system) : system{system}
{
}

void PhysicsQueries::raycast(const Float3& from, const Float3& to, std::vector<PhysicsHit>& hits) const
{
    const JPH::RRayCast ray{toJoltVector(from), toJoltVector(to) - toJoltVector(from)};

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;

    system.GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings{}, collector);

    std::sort(collector.mHits.begin(), collector.mHits.end(), nearestFirst<JPH::RayCastResult>);

    hits.clear();
    hits.reserve(collector.mHits.size());

    for (const JPH::RayCastResult& hit : collector.mHits)
    {
        hits.push_back(PhysicsHit{toBodyId(hit.mBodyID), hit.mFraction});
    }
}

void PhysicsQueries::overlapSphere(const Float3& centre, float radius, std::vector<BodyId>& bodies) const
{
    const JPH::Ref<JPH::Shape> shape = sphereShape(radius);
    const JPH::RVec3 at = toJoltVector(centre);

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    system.GetNarrowPhaseQuery().CollideShape(
        shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(at), JPH::CollideShapeSettings{}, at, collector);

    std::sort(collector.mHits.begin(), collector.mHits.end(), byBodyThenSubShape<JPH::CollideShapeResult>);

    bodies.clear();
    bodies.reserve(collector.mHits.size());

    for (const JPH::CollideShapeResult& hit : collector.mHits)
    {
        bodies.push_back(toBodyId(hit.mBodyID2));
    }
}

void PhysicsQueries::sweepCapsule(
    const Float3& from, const Float3& to, float radius, float halfHeight, std::vector<PhysicsHit>& hits) const
{
    const JPH::Ref<JPH::Shape> shape = capsuleShape(halfHeight, radius);
    const JPH::RVec3 at = toJoltVector(from);
    const JPH::RShapeCast sweep = JPH::RShapeCast::sFromWorldTransform(
        shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(at), toJoltVector(to) - at);

    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;

    system.GetNarrowPhaseQuery().CastShape(sweep, JPH::ShapeCastSettings{}, at, collector);

    std::sort(collector.mHits.begin(), collector.mHits.end(), nearestFirst<JPH::ShapeCastResult>);

    hits.clear();
    hits.reserve(collector.mHits.size());

    for (const JPH::ShapeCastResult& hit : collector.mHits)
    {
        hits.push_back(PhysicsHit{toBodyId(hit.mBodyID2), hit.mFraction});
    }
}

}
