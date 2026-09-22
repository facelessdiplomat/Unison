#include <unison/sim/jolt_runtime.hpp>

#include <unison/core/contract.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <memory>

namespace unison::sim
{

namespace
{

int openScopes = 0;
std::unique_ptr<JPH::Factory> factory;

}

JoltRuntime::JoltRuntime()
{
    if (openScopes == 0)
    {
        JPH::RegisterDefaultAllocator();
        factory = std::make_unique<JPH::Factory>();
        JPH::Factory::sInstance = factory.get();
        JPH::RegisterTypes();
    }

    ++openScopes;
}

JoltRuntime::~JoltRuntime()
{
    UNISON_ASSERT(openScopes > 0);

    --openScopes;

    if (openScopes == 0)
    {
        JPH::UnregisterTypes();
        JPH::Factory::sInstance = nullptr;
        factory.reset();
    }
}

}
