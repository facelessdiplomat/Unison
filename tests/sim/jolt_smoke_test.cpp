#include <catch2/catch_test_macros.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

namespace
{

constexpr JPH::ObjectLayer kStaticLayer = 0;
constexpr JPH::ObjectLayer kMovingLayer = 1;
constexpr JPH::uint kBroadPhaseLayerCount = 2;

class BroadPhaseLayerMapping final : public JPH::BroadPhaseLayerInterface
{
public:
    JPH::uint GetNumBroadPhaseLayers() const override
    {
        return kBroadPhaseLayerCount;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return JPH::BroadPhaseLayer(static_cast<JPH::BroadPhaseLayer::Type>(layer == kStaticLayer ? 0 : 1));
    }
};

class EverythingCollidesWithBroadPhase final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override
    {
        return true;
    }
};

class EverythingCollides final : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override
    {
        return true;
    }
};

}

TEST_CASE("jolt is built for cross platform determinism")
{
#ifdef JPH_CROSS_PLATFORM_DETERMINISTIC
    SUCCEED();
#else
    FAIL("JPH_CROSS_PLATFORM_DETERMINISTIC is not defined");
#endif
}

TEST_CASE("a physics system steps once")
{
    JPH::RegisterDefaultAllocator();

    static JPH::Factory factory;
    JPH::Factory::sInstance = &factory;
    JPH::RegisterTypes();

    JPH::TempAllocatorImpl tempAllocator(1024 * 1024);
    JPH::JobSystemSingleThreaded jobSystem(JPH::cMaxPhysicsJobs);

    const BroadPhaseLayerMapping broadPhaseLayers;
    const EverythingCollidesWithBroadPhase objectVsBroadPhaseFilter;
    const EverythingCollides objectLayerPairFilter;

    JPH::PhysicsSystem physicsSystem;
    physicsSystem.Init(16, 0, 64, 64, broadPhaseLayers, objectVsBroadPhaseFilter, objectLayerPairFilter);

    const JPH::EPhysicsUpdateError error = physicsSystem.Update(1.0f / 60.0f, 1, &tempAllocator, &jobSystem);

    REQUIRE(error == JPH::EPhysicsUpdateError::None);
    REQUIRE(physicsSystem.GetNumBodies() == 0U);

    JPH::UnregisterTypes();
    JPH::Factory::sInstance = nullptr;
}
