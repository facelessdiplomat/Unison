#include <unison/sim/padding_free.hpp>

#include <unison/core/float3.hpp>
#include <unison/core/quaternion.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>

namespace
{

struct TwoFloats
{
    float x = 0.0F;
    float y = 0.0F;
};

struct PaddedPair
{
    std::uint8_t small = 0;
    std::uint32_t large = 0;
};

struct Transform
{
    unison::Float3 position{};
    unison::Quaternion rotation{};
};

struct HoldingPaddedMember
{
    PaddedPair padded{};
    std::uint32_t extra = 0;
};

struct Tag
{
};

}

TEST_CASE("a type whose members fill it completely is padding free")
{
    STATIC_REQUIRE(unison::sim::PaddingFree<TwoFloats>);
    STATIC_REQUIRE(unison::sim::PaddingFree<std::uint32_t>);
    STATIC_REQUIRE(unison::sim::PaddingFree<std::array<float, 3>>);
    STATIC_REQUIRE(unison::sim::PaddingFree<Tag>);
}

TEST_CASE("a type the compiler padded for alignment is not padding free")
{
    STATIC_REQUIRE_FALSE(unison::sim::PaddingFree<PaddedPair>);
}

TEST_CASE("padding hiding inside a member is found too")
{
    STATIC_REQUIRE(sizeof(HoldingPaddedMember) == sizeof(PaddedPair) + sizeof(std::uint32_t));
    STATIC_REQUIRE_FALSE(unison::sim::PaddingFree<HoldingPaddedMember>);
}

TEST_CASE("the engine's own math storage types are padding free")
{
    STATIC_REQUIRE(unison::sim::PaddingFree<unison::Float3>);
    STATIC_REQUIRE(unison::sim::PaddingFree<unison::Quaternion>);
    STATIC_REQUIRE(unison::sim::PaddingFree<Transform>);
}
