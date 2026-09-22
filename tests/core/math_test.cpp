#include <unison/core/math.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>

namespace
{

struct GoldenAngle
{
    float angle;
    std::uint32_t sineBits;
    std::uint32_t cosineBits;
};

struct GoldenArcTangent
{
    float y;
    float x;
    std::uint32_t bits;
};

constexpr std::array<GoldenAngle, 11> kGoldenAngles{{
    {-6.282372F, 0x3A553442U, 0x3F7FFFFAU},
    {-6.27923012F, 0x3B819A73U, 0x3F7FFF7DU},
    {-6.27451801F, 0x3C0E00CFU, 0x3F7FFD8AU},
    {-3.0F, 0xBE1081C3U, 0xBF7D7026U},
    {-1.5F, 0xBF7F5BD5U, 0x3D90DEAAU},
    {-0.25F, 0xBE7D5777U, 0x3F780AA5U},
    {0.0F, 0x00000000U, 0x3F800000U},
    {0.25F, 0x3E7D5777U, 0x3F780AA5U},
    {1.0F, 0x3F576AA4U, 0x3F0A5140U},
    {1.5F, 0x3F7F5BD5U, 0x3D90DEAAU},
    {3.0F, 0x3E1081C3U, 0xBF7D7026U},
}};

constexpr std::array<GoldenArcTangent, 4> kGoldenArcTangents{{
    {-1.0F, -1.0F, 0xC016CBE4U},
    {-1.0F, 1.0F, 0xBF490FDBU},
    {1.0F, -1.0F, 0x4016CBE4U},
    {1.0F, 1.0F, 0x3F490FDBU},
}};

constexpr double kTolerance = 1e-6;

std::uint32_t bitsOf(float value)
{
    return std::bit_cast<std::uint32_t>(value);
}

}

TEST_CASE("math sine and cosine match their recorded bit patterns")
{
    for (const GoldenAngle& golden : kGoldenAngles)
    {
        REQUIRE(bitsOf(unison::math::sin(golden.angle)) == golden.sineBits);
        REQUIRE(bitsOf(unison::math::cos(golden.angle)) == golden.cosineBits);
    }
}

TEST_CASE("math arc tangent matches its recorded bit patterns")
{
    for (const GoldenArcTangent& golden : kGoldenArcTangents)
    {
        REQUIRE(bitsOf(unison::math::atan2(golden.y, golden.x)) == golden.bits);
    }
}

TEST_CASE("math sine agrees with the standard library")
{
    for (const GoldenAngle& golden : kGoldenAngles)
    {
        const double reference = std::sin(static_cast<double>(golden.angle));

        REQUIRE_THAT(unison::math::sin(golden.angle), Catch::Matchers::WithinAbs(reference, kTolerance));
    }
}

TEST_CASE("math cosine agrees with the standard library")
{
    for (const GoldenAngle& golden : kGoldenAngles)
    {
        const double reference = std::cos(static_cast<double>(golden.angle));

        REQUIRE_THAT(unison::math::cos(golden.angle), Catch::Matchers::WithinAbs(reference, kTolerance));
    }
}

TEST_CASE("math arc tangent agrees with the standard library in every quadrant")
{
    for (const GoldenArcTangent& golden : kGoldenArcTangents)
    {
        const double reference = std::atan2(static_cast<double>(golden.y), static_cast<double>(golden.x));

        REQUIRE_THAT(unison::math::atan2(golden.y, golden.x), Catch::Matchers::WithinAbs(reference, kTolerance));
    }
}

TEST_CASE("math square root is the correctly rounded result")
{
    for (const float value : {0.0F, 0.25F, 1.0F, 2.0F, 1024.0F})
    {
        REQUIRE(bitsOf(unison::math::sqrt(value)) == bitsOf(std::sqrt(value)));
    }
}

TEST_CASE("math lerp hits both endpoints and the midpoint exactly")
{
    REQUIRE(bitsOf(unison::math::lerp(1.0F, 3.0F, 0.0F)) == bitsOf(1.0F));
    REQUIRE(bitsOf(unison::math::lerp(1.0F, 3.0F, 0.5F)) == bitsOf(2.0F));
    REQUIRE(bitsOf(unison::math::lerp(1.0F, 3.0F, 1.0F)) == bitsOf(3.0F));
}

TEST_CASE("math clamp keeps a value inside its bounds")
{
    REQUIRE(bitsOf(unison::math::clamp(-1.0F, 0.0F, 2.0F)) == bitsOf(0.0F));
    REQUIRE(bitsOf(unison::math::clamp(1.0F, 0.0F, 2.0F)) == bitsOf(1.0F));
    REQUIRE(bitsOf(unison::math::clamp(3.0F, 0.0F, 2.0F)) == bitsOf(2.0F));
}

TEST_CASE("math clamp and lerp are usable at compile time")
{
    STATIC_REQUIRE(unison::math::clamp(3.0F, 0.0F, 2.0F) == 2.0F);
    STATIC_REQUIRE(unison::math::lerp(1.0F, 3.0F, 0.5F) == 2.0F);
}
