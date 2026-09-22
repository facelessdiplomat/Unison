#include <unison/core/raw_value.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace
{

struct Pod
{
    std::uint32_t left;
    std::uint32_t right;
};

}

TEST_CASE("a raw value carries its whole meaning in its bytes")
{
    STATIC_REQUIRE(unison::RawValue<std::uint32_t>);
    STATIC_REQUIRE(unison::RawValue<Pod>);
    STATIC_REQUIRE(unison::RawValue<std::array<std::uint32_t, 4>>);
}

TEST_CASE("a type whose bytes hold an address is not a raw value")
{
    STATIC_REQUIRE_FALSE(unison::RawValue<const std::byte*>);
    STATIC_REQUIRE_FALSE(unison::RawValue<std::span<const std::byte>>);
    STATIC_REQUIRE_FALSE(unison::RawValue<std::span<const std::byte, 4>>);
    STATIC_REQUIRE_FALSE(unison::RawValue<std::string_view>);
}

TEST_CASE("a type that owns memory is not a raw value")
{
    STATIC_REQUIRE_FALSE(unison::RawValue<std::vector<std::uint32_t>>);
}
