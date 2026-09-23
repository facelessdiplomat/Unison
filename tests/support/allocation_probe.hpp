#pragma once

#include <cstddef>

namespace unison::test
{

/// Counts the heap allocations made while it is in scope: the C++ heap through the global operator new,
/// which the containers and EnTT allocate with, and Jolt's heap through the functions Jolt allocates
/// with. Jolt's functions are wrapped for the length of the scope and put back on exit, so a probe is
/// created after the physics worlds it watches, which install them.
class AllocationProbe
{
public:
    AllocationProbe();
    ~AllocationProbe();

    AllocationProbe(const AllocationProbe&) = delete;
    AllocationProbe& operator=(const AllocationProbe&) = delete;
    AllocationProbe(AllocationProbe&&) = delete;
    AllocationProbe& operator=(AllocationProbe&&) = delete;

    [[nodiscard]] std::size_t cppAllocations() const;

    [[nodiscard]] std::size_t joltAllocations() const;

    [[nodiscard]] std::size_t allocations() const;
};

}
