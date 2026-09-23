#pragma once

#include <unison/core/float3.hpp>
#include <unison/sim/body_table.hpp>
#include <unison/sim/character_table.hpp>
#include <unison/sim/contact.hpp>
#include <unison/sim/jolt_contacts.hpp>
#include <unison/sim/jolt_layers.hpp>
#include <unison/sim/jolt_runtime.hpp>
#include <unison/sim/physics_queries.hpp>

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

/// A Jolt physics system with everything that keeps it reproducible: one job thread, one fixed block
/// of scratch memory, and the layer rules that decide which bodies may meet. It advances the bodies and
/// characters of one frame a tick at a time and saves and restores their state; building them and asking
/// where they are goes through its bodies, its characters and its queries, as Jolt's own system does.
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

    [[nodiscard]] BodyTable& bodies();

    [[nodiscard]] const BodyTable& bodies() const;

    [[nodiscard]] CharacterTable& characters();

    [[nodiscard]] const CharacterTable& characters() const;

    [[nodiscard]] const PhysicsQueries& queries() const;

    /// The bodies that touched during the last step, in body order.
    [[nodiscard]] std::span<const Contact> contacts() const;

    /// Writes everything Jolt keeps about this world into the bytes, replacing what they held.
    void saveState(std::vector<std::byte>& bytes) const;

    /// Puts the world back to where the bytes were written, which it can only do while it still
    /// holds the same bodies it held then.
    void restoreState(std::span<const std::byte> bytes);

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
    BodyTable bodyTable;
    CharacterTable characterTable;
    PhysicsQueries physicsQueries;
};

}
