#pragma once

#include <unison/core/body_id.hpp>
#include <unison/core/float3.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/PhysicsSystem.h>

#include <optional>
#include <vector>

namespace unison::sim
{

/// One thing a query found: the body it met and how far along the query it stood, nought for a query
/// that only asks what overlaps where it is.
struct PhysicsHit
{
    BodyId body = BodyId::Invalid;
    float fraction = 0.0F;
};

/// The questions a system may ask about where the bodies of a world are. Jolt answers in whatever order
/// its traversal reaches them, so every answer is sorted before it is handed out, and every client reads
/// the same list. A query fills the vector it is given, and allocates nothing once that vector has held as
/// many answers; a sphere or a capsule is built afresh for each query, which Jolt allocates.
class PhysicsQueries
{
public:
    explicit PhysicsQueries(const JPH::PhysicsSystem& system);

    /// The bodies a ray meets on its way from one point to the other, nearest first and in body order
    /// where they are equally near.
    void raycast(const Float3& from, const Float3& to, std::vector<PhysicsHit>& hits) const;

    /// The body a ray meets first on its way from one point to the other, the lowest one where several
    /// are equally near, or nothing when it meets none.
    [[nodiscard]] std::optional<PhysicsHit> raycastNearest(const Float3& from, const Float3& to) const;

    /// The bodies a sphere overlaps where it stands, in body order.
    void overlapSphere(const Float3& centre, float radius, std::vector<BodyId>& bodies) const;

    /// The bodies an upright capsule meets as it sweeps from one point to the other, nearest first.
    void sweepCapsule(
        const Float3& from, const Float3& to, float radius, float halfHeight, std::vector<PhysicsHit>& hits) const;

private:
    const JPH::PhysicsSystem& system;
};

}
