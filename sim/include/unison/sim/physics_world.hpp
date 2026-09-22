#pragma once

#include <unison/core/float3.hpp>
#include <unison/sim/jolt_runtime.hpp>
#include <unison/sim/physics_layers.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cstdint>

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

    [[nodiscard]] std::uint32_t bodyCount() const;

    [[nodiscard]] Float3 gravity() const;

private:
    JoltRuntime runtime;
    BroadPhaseLayerMapping broadPhaseLayers;
    ObjectVsBroadPhaseFilter objectVsBroadPhaseFilter;
    ObjectPairFilter objectPairFilter;
    JPH::TempAllocatorImpl scratchAllocator;
    JPH::JobSystemSingleThreaded jobSystem;
    JPH::PhysicsSystem physicsSystem;
};

}
