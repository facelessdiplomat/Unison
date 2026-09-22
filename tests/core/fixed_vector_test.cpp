#include <unison/core/fixed_vector.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <type_traits>

namespace
{

using SmallVector = unison::FixedVector<std::int32_t, 4>;

SmallVector makeVector(std::initializer_list<std::int32_t> values)
{
    SmallVector vector{};

    for (const std::int32_t value : values)
    {
        vector.pushBack(value);
    }

    return vector;
}

}

TEST_CASE("fixed vector starts empty")
{
    const SmallVector vector{};

    REQUIRE(vector.size() == 0U);
    REQUIRE_FALSE(vector.isFull());
}

TEST_CASE("fixed vector appends elements in order")
{
    SmallVector vector{};

    vector.pushBack(10);
    vector.pushBack(20);
    vector.pushBack(30);

    REQUIRE(vector.size() == 3U);
    REQUIRE(vector[0] == 10);
    REQUIRE(vector[1] == 20);
    REQUIRE(vector[2] == 30);
}

TEST_CASE("fixed vector drops the last element on pop")
{
    SmallVector vector = makeVector({10, 20, 30});

    vector.popBack();

    REQUIRE(vector.size() == 2U);
    REQUIRE(vector[1] == 20);
}

TEST_CASE("fixed vector reports full when it holds its capacity")
{
    SmallVector vector = makeVector({10, 20, 30});

    REQUIRE_FALSE(vector.isFull());

    vector.pushBack(40);

    REQUIRE(vector.isFull());
    REQUIRE(vector.size() == SmallVector::kCapacity);
}

TEST_CASE("fixed vector writes through the index operator")
{
    SmallVector vector = makeVector({10, 20});

    vector[1] = 25;

    REQUIRE(vector[1] == 25);
}

TEST_CASE("fixed vector iterates over the elements it holds")
{
    const SmallVector vector = makeVector({10, 20, 30});

    std::int32_t sum = 0;
    std::size_t visited = 0;

    for (const std::int32_t value : vector)
    {
        sum += value;
        ++visited;
    }

    REQUIRE(visited == 3U);
    REQUIRE(sum == 60);
}

TEST_CASE("fixed vector is empty after clear")
{
    SmallVector vector = makeVector({10, 20, 30});

    vector.clear();

    REQUIRE(vector.size() == 0U);
    REQUIRE(vector.begin() == vector.end());
}

TEST_CASE("fixed vector is trivially copyable when its element is")
{
    STATIC_REQUIRE(std::is_trivially_copyable_v<SmallVector>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<unison::FixedVector<SmallVector, 2>>);
}

TEST_CASE("fixed vector byte image depends only on the elements it holds")
{
    SmallVector popped = makeVector({10, 20, 30});
    popped.popBack();
    popped.popBack();

    SmallVector reused = makeVector({40, 50, 60, 70});
    reused.clear();
    reused.pushBack(10);

    const SmallVector pushed = makeVector({10});

    REQUIRE(std::memcmp(&popped, &pushed, sizeof(SmallVector)) == 0);
    REQUIRE(std::memcmp(&reused, &pushed, sizeof(SmallVector)) == 0);
}
