#include <unison/core/body_id.hpp>

#include <unison/core/jolt_conversions.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

TEST_CASE("a body id carries the index and the sequence it was made from")
{
    const unison::BodyId id = unison::makeBodyId(7U, 3U);

    REQUIRE(unison::bodyIndexOf(id) == 7U);
    REQUIRE(unison::bodySequenceOf(id) == 3U);
}

TEST_CASE("ids of the same slot in different rounds are different ids")
{
    REQUIRE(unison::makeBodyId(7U, 3U) != unison::makeBodyId(7U, 4U));
}

TEST_CASE("no id of a body equals the invalid id")
{
    REQUIRE(unison::makeBodyId(unison::kMaxBodyIndex, 0xFFU) != unison::BodyId::Invalid);
    REQUIRE(unison::makeBodyId(0U, 0U) != unison::BodyId::Invalid);
}

TEST_CASE("every body the engine allows has an index to name it")
{
    STATIC_REQUIRE(unison::kMaxBodies <= unison::kMaxBodyIndex + 1U);
}

TEST_CASE("a body id round trips through the id jolt knows the body by")
{
    const unison::BodyId id = unison::makeBodyId(11U, 5U);

    REQUIRE(unison::toBodyId(unison::toJoltBodyId(id)) == id);
    REQUIRE(unison::toJoltBodyId(id).GetIndex() == 11U);
    REQUIRE(unison::toJoltBodyId(id).GetSequenceNumber() == 5U);
}
