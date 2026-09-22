#pragma once

#include <unison/core/body_id.hpp>
#include <unison/core/float3.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/character_table.hpp>
#include <unison/sim/contact.hpp>
#include <unison/sim/jolt_contacts.hpp>
#include <unison/sim/jolt_layers.hpp>
#include <unison/sim/jolt_runtime.hpp>
#include <unison/sim/transform.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unison::sim
{

/// What a physics world is built with. Jolt reserves for these limits once and never grows, so they
/// belong to the game's configuration; gravity is in metres per second squared along a Y-up axis.
struct PhysicsWorldSettings
{
    std::uint32_t maxBodyPairs = 4096;
    std::uint32_t maxContactConstraints = 2048;
    std::uint32_t scratchBytes = 8U * 1024U * 1024U;
    Float3 gravity{0.0F, -9.81F, 0.0F};
};

/// One thing a query found: the body it met and how far along the query it stood, nought for a query
/// that only asks what overlaps where it is.
struct PhysicsHit
{
    BodyId body = BodyId::Invalid;
    float fraction = 0.0F;
};

/// A Jolt physics system with everything that keeps it reproducible: one job thread, one fixed block
/// of scratch memory, and the layer rules that decide which bodies may meet. It advances the bodies
/// of one frame a tick at a time and owns nothing else about the frame.
class PhysicsWorld
{
public:
    explicit PhysicsWorld(const PhysicsWorldSettings& settings = PhysicsWorldSettings{});

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    PhysicsWorld(PhysicsWorld&&) = delete;
    PhysicsWorld& operator=(PhysicsWorld&&) = delete;

    /// Advances every body by one tick. It expects the floating-point environment `advanceFrame`
    /// installs, and an update Jolt could not complete within its limits breaks a contract.
    void step(float dt);

    /// Builds the body a definition describes, placed where the transform says, under the id the
    /// frame handed out. No body of this world may carry that id yet.
    void createBody(BodyId id, const BodyDefinition& definition, const Transform& placement);

    /// Takes a body of this world out of it, leaving its id free to be handed out again.
    void destroyBody(BodyId id);

    [[nodiscard]] bool holdsBody(BodyId id) const;

    [[nodiscard]] Transform transformOf(BodyId id) const;

    [[nodiscard]] CharacterTable& characters();

    [[nodiscard]] const CharacterTable& characters() const;

    /// The bodies that touched during the last step, in body order.
    [[nodiscard]] std::span<const Contact> contacts() const;

    /// The bodies a ray meets on its way from one point to the other, nearest first and in body order
    /// where they are equally near, so every client reads the same list.
    void raycast(const Float3& from, const Float3& to, std::vector<PhysicsHit>& hits) const;

    /// The bodies a sphere overlaps where it stands, in body order.
    void overlapSphere(const Float3& centre, float radius, std::vector<BodyId>& bodies) const;

    /// The bodies an upright capsule meets as it sweeps from one point to the other, nearest first.
    void sweepCapsule(
        const Float3& from, const Float3& to, float radius, float halfHeight, std::vector<PhysicsHit>& hits) const;

    /// Puts back on a body what Jolt leaves out of the state buffer it saves.
    void applyProperties(BodyId id, const BodyDefinition& definition);

    [[nodiscard]] float frictionOf(BodyId id) const;

    [[nodiscard]] float restitutionOf(BodyId id) const;

    [[nodiscard]] BodyMotion motionOf(BodyId id) const;

    /// Names every body the world holds, in the order Jolt keeps them.
    void collectBodies(std::vector<BodyId>& bodies) const;

    /// Writes everything Jolt keeps about this world into the bytes, replacing what they held.
    void saveState(std::vector<std::byte>& bytes) const;

    /// Puts the world back to where the bytes were written, which it can only do while it still
    /// holds the same bodies it held then.
    void restoreState(std::span<const std::byte> bytes);

    [[nodiscard]] std::uint32_t bodyCount() const;

    [[nodiscard]] Float3 gravity() const;

private:
    JoltRuntime runtime;
    BroadPhaseLayerMapping broadPhaseLayers;
    ObjectVsBroadPhaseFilter objectVsBroadPhaseFilter;
    ObjectPairFilter objectPairFilter;
    JPH::TempAllocatorImpl scratchAllocator;
    JPH::JobSystemSingleThreaded jobSystem;
    ContactCollector contactCollector;
    JPH::PhysicsSystem physicsSystem;
    CharacterTable characterTable;
};

}
