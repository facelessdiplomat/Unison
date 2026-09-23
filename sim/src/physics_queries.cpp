#include <unison/sim/physics_queries.hpp>

#include <unison/core/jolt_conversions.hpp>
#include <unison/sim/jolt_shapes.hpp>

#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

#include <algorithm>

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

bool isNearer(const PhysicsHit& first, const PhysicsHit& second)
{
    if (first.fraction != second.fraction)
    {
        return first.fraction < second.fraction;
    }

    return first.body < second.body;
}

template <typename Collector>
class HitList final : public Collector
{
public:
    explicit HitList(std::vector<PhysicsHit>& hits) : hits{hits}
    {
    }

    void AddHit(const typename Collector::ResultType& hit) override
    {
        hits.push_back(PhysicsHit{toBodyId(bodyOf(hit)), hit.mFraction});
    }

private:
    std::vector<PhysicsHit>& hits;
};

class NearestRayHit final : public JPH::CastRayCollector
{
public:
    void AddHit(const JPH::RayCastResult& hit) override
    {
        const PhysicsHit candidate{toBodyId(hit.mBodyID), hit.mFraction};

        if (!nearest.has_value() || isNearer(candidate, *nearest))
        {
            nearest = candidate;
        }
    }

    [[nodiscard]] std::optional<PhysicsHit> found() const
    {
        return nearest;
    }

private:
    std::optional<PhysicsHit> nearest;
};

class OverlappedBodies final : public JPH::CollideShapeCollector
{
public:
    explicit OverlappedBodies(std::vector<BodyId>& bodies) : bodies{bodies}
    {
    }

    void AddHit(const JPH::CollideShapeResult& hit) override
    {
        bodies.push_back(toBodyId(hit.mBodyID2));
    }

private:
    std::vector<BodyId>& bodies;
};

JPH::RRayCast rayBetween(const Float3& from, const Float3& to)
{
    return JPH::RRayCast{toJoltVector(from), toJoltVector(to) - toJoltVector(from)};
}

}

PhysicsQueries::PhysicsQueries(const JPH::PhysicsSystem& system) : system{system}
{
}

void PhysicsQueries::raycast(const Float3& from, const Float3& to, std::vector<PhysicsHit>& hits) const
{
    hits.clear();

    HitList<JPH::CastRayCollector> collector{hits};

    system.GetNarrowPhaseQuery().CastRay(rayBetween(from, to), JPH::RayCastSettings{}, collector);

    std::sort(hits.begin(), hits.end(), isNearer);
}

std::optional<PhysicsHit> PhysicsQueries::raycastNearest(const Float3& from, const Float3& to) const
{
    NearestRayHit collector;

    system.GetNarrowPhaseQuery().CastRay(rayBetween(from, to), JPH::RayCastSettings{}, collector);

    return collector.found();
}

void PhysicsQueries::overlapSphere(const Float3& centre, float radius, std::vector<BodyId>& bodies) const
{
    const JPH::Ref<JPH::Shape> shape = sphereShape(radius);
    const JPH::RVec3 at = toJoltVector(centre);

    bodies.clear();

    OverlappedBodies collector{bodies};

    system.GetNarrowPhaseQuery().CollideShape(
        shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(at), JPH::CollideShapeSettings{}, at, collector);

    std::sort(bodies.begin(), bodies.end());
}

void PhysicsQueries::sweepCapsule(
    const Float3& from, const Float3& to, float radius, float halfHeight, std::vector<PhysicsHit>& hits) const
{
    const JPH::Ref<JPH::Shape> shape = capsuleShape(halfHeight, radius);
    const JPH::RVec3 at = toJoltVector(from);
    const JPH::RShapeCast sweep = JPH::RShapeCast::sFromWorldTransform(
        shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(at), toJoltVector(to) - at);

    hits.clear();

    HitList<JPH::CastShapeCollector> collector{hits};

    system.GetNarrowPhaseQuery().CastShape(sweep, JPH::ShapeCastSettings{}, at, collector);

    std::sort(hits.begin(), hits.end(), isNearer);
}

}
