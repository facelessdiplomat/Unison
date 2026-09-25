#include <support/allocation_probe.hpp>

#include <Jolt/Jolt.h>

#include <algorithm>
#include <cstdlib>
#include <new>

#ifdef _WIN32
    #include <malloc.h>
#endif

namespace
{

bool isCounting = false;
std::size_t cppCount = 0;
std::size_t joltCount = 0;

JPH::AllocateFunction joltAllocate = nullptr;
JPH::ReallocateFunction joltReallocate = nullptr;
JPH::AlignedAllocateFunction joltAlignedAllocate = nullptr;

void* countedJoltAllocate(std::size_t size)
{
    ++joltCount;

    return joltAllocate(size);
}

void* countedJoltReallocate(void* block, std::size_t oldSize, std::size_t newSize)
{
    ++joltCount;

    return joltReallocate(block, oldSize, newSize);
}

void* countedJoltAlignedAllocate(std::size_t size, std::size_t alignment)
{
    ++joltCount;

    return joltAlignedAllocate(size, alignment);
}

void* allocateAlignedBlock(std::size_t size, std::size_t alignment)
{
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* block = nullptr;

    return posix_memalign(&block, std::max(alignment, sizeof(void*)), size) == 0 ? block : nullptr;
#endif
}

void freeAlignedBlock(void* block)
{
#ifdef _WIN32
    _aligned_free(block);
#else
    std::free(block);
#endif
}

void countCppAllocation()
{
    if (isCounting)
    {
        ++cppCount;
    }
}

void* allocateOrThrow(std::size_t size)
{
    countCppAllocation();

    void* block = std::malloc(size == 0 ? 1 : size);

    if (block == nullptr)
    {
        throw std::bad_alloc{};
    }

    return block;
}

void* allocateAlignedOrThrow(std::size_t size, std::align_val_t alignment)
{
    countCppAllocation();

    void* block = allocateAlignedBlock(size == 0 ? 1 : size, static_cast<std::size_t>(alignment));

    if (block == nullptr)
    {
        throw std::bad_alloc{};
    }

    return block;
}

}

void* operator new(std::size_t size)
{
    return allocateOrThrow(size);
}

void* operator new[](std::size_t size)
{
    return allocateOrThrow(size);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
    return allocateAlignedOrThrow(size, alignment);
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return allocateAlignedOrThrow(size, alignment);
}

void operator delete(void* block) noexcept
{
    std::free(block);
}

void operator delete[](void* block) noexcept
{
    std::free(block);
}

void operator delete(void* block, std::size_t) noexcept
{
    std::free(block);
}

void operator delete[](void* block, std::size_t) noexcept
{
    std::free(block);
}

void operator delete(void* block, std::align_val_t) noexcept
{
    freeAlignedBlock(block);
}

void operator delete[](void* block, std::align_val_t) noexcept
{
    freeAlignedBlock(block);
}

void operator delete(void* block, std::size_t, std::align_val_t) noexcept
{
    freeAlignedBlock(block);
}

void operator delete[](void* block, std::size_t, std::align_val_t) noexcept
{
    freeAlignedBlock(block);
}

namespace unison::test
{

AllocationProbe::AllocationProbe()
{
    cppCount = 0;
    joltCount = 0;

    joltAllocate = JPH::Allocate;
    joltReallocate = JPH::Reallocate;
    joltAlignedAllocate = JPH::AlignedAllocate;

    JPH::Allocate = &countedJoltAllocate;
    JPH::Reallocate = &countedJoltReallocate;
    JPH::AlignedAllocate = &countedJoltAlignedAllocate;

    isCounting = true;
}

AllocationProbe::~AllocationProbe()
{
    isCounting = false;

    JPH::Allocate = joltAllocate;
    JPH::Reallocate = joltReallocate;
    JPH::AlignedAllocate = joltAlignedAllocate;
}

std::size_t AllocationProbe::cppAllocations() const
{
    return cppCount;
}

std::size_t AllocationProbe::joltAllocations() const
{
    return joltCount;
}

std::size_t AllocationProbe::allocations() const
{
    return cppCount + joltCount;
}

}
