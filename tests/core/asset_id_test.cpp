#include <unison/core/asset_id.hpp>
#include <unison/core/raw_value.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <type_traits>

namespace
{

template <unison::AssetId Id>
constexpr unison::AssetId throughTemplateArgument()
{
    return Id;
}

constexpr int classify(unison::AssetId id)
{
    switch (id)
    {
        case unison::makeAssetId("floor"):
            return 1;
        case unison::makeAssetId("wall"):
            return 2;
        default:
            return 0;
    }
}

constexpr unison::AssetId asAssetId(std::uint32_t value)
{
    return static_cast<unison::AssetId>(value);
}

}

TEST_CASE("asset id matches the published FNV-1a 32 vectors")
{
    STATIC_REQUIRE(unison::makeAssetId("") == asAssetId(0x811C9DC5U));
    STATIC_REQUIRE(unison::makeAssetId("a") == asAssetId(0xE40C292CU));
    STATIC_REQUIRE(unison::makeAssetId("b") == asAssetId(0xE70C2DE5U));
    STATIC_REQUIRE(unison::makeAssetId("foobar") == asAssetId(0xBF9CF968U));
}

TEST_CASE("asset ids of distinct names differ")
{
    STATIC_REQUIRE(unison::makeAssetId("floor") != unison::makeAssetId("wall"));
    STATIC_REQUIRE(unison::makeAssetId("crate") != unison::makeAssetId("crates"));
}

TEST_CASE("asset id is usable as a non-type template argument")
{
    STATIC_REQUIRE(throughTemplateArgument<unison::makeAssetId("floor")>() == unison::makeAssetId("floor"));
}

TEST_CASE("asset id is usable as a switch label")
{
    STATIC_REQUIRE(classify(unison::makeAssetId("floor")) == 1);
    STATIC_REQUIRE(classify(unison::makeAssetId("wall")) == 2);
    STATIC_REQUIRE(classify(unison::makeAssetId("ramp")) == 0);
}

TEST_CASE("asset id is a 32-bit raw value")
{
    STATIC_REQUIRE(std::is_same_v<std::underlying_type_t<unison::AssetId>, std::uint32_t>);
    STATIC_REQUIRE(unison::RawValue<unison::AssetId>);
}
