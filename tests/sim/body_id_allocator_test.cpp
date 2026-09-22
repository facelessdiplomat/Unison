#include <unison/sim/body_id_allocator.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

TEST_CASE("a fresh allocator counts its indices up from zero")
{
    unison::sim::BodyIdAllocator allocator;

    REQUIRE(allocator.allocate() == unison::makeBodyId(0U, 0U));
    REQUIRE(allocator.allocate() == unison::makeBodyId(1U, 0U));
    REQUIRE(allocator.allocate() == unison::makeBodyId(2U, 0U));
}

TEST_CASE("a released id comes back with the next sequence number")
{
    unison::sim::BodyIdAllocator allocator;

    static_cast<void>(allocator.allocate());
    const unison::BodyId released = allocator.allocate();

    allocator.release(released);

    REQUIRE(allocator.allocate() == unison::makeBodyId(1U, 1U));
}

TEST_CASE("released indices are taken before fresh ones")
{
    unison::sim::BodyIdAllocator allocator;

    const unison::BodyId first = allocator.allocate();
    static_cast<void>(allocator.allocate());

    allocator.release(first);

    REQUIRE(unison::bodyIndexOf(allocator.allocate()) == 0U);
    REQUIRE(unison::bodyIndexOf(allocator.allocate()) == 2U);
}

TEST_CASE("the id released last is the first one handed out again")
{
    unison::sim::BodyIdAllocator allocator;

    const unison::BodyId first = allocator.allocate();
    const unison::BodyId second = allocator.allocate();

    allocator.release(first);
    allocator.release(second);

    REQUIRE(unison::bodyIndexOf(allocator.allocate()) == 1U);
    REQUIRE(unison::bodyIndexOf(allocator.allocate()) == 0U);
}

TEST_CASE("two allocators given the same calls hand out the same ids")
{
    unison::sim::BodyIdAllocator one;
    unison::sim::BodyIdAllocator other;

    for (int round = 0; round < 3; ++round)
    {
        const unison::BodyId fromOne = one.allocate();
        const unison::BodyId fromOther = other.allocate();

        REQUIRE(fromOne == fromOther);

        one.release(fromOne);
        other.release(fromOther);
    }

    const unison::BodyId last = one.allocate();

    REQUIRE(last == other.allocate());
    REQUIRE(last == unison::makeBodyId(0U, 3U));
}

TEST_CASE("a copied allocator carries on where the original stood")
{
    unison::sim::BodyIdAllocator allocator;

    allocator.release(allocator.allocate());

    unison::sim::BodyIdAllocator copy = allocator;

    REQUIRE(copy.allocate() == unison::makeBodyId(0U, 1U));
}

TEST_CASE("an allocator remembers the slots it took and the ids it has back")
{
    unison::sim::BodyIdAllocator allocator;

    const unison::BodyId first = allocator.allocate();
    const unison::BodyId second = allocator.allocate();

    allocator.release(second);
    allocator.release(first);

    REQUIRE(allocator.takenSlotCount() == 2U);
    REQUIRE(allocator.freeIds().size() == 2U);
    REQUIRE(allocator.freeIds()[0] == second);
    REQUIRE(allocator.freeIds()[1] == first);
}
