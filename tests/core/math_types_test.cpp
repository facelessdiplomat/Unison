#include <unison/core/float3.hpp>
#include <unison/core/jolt_conversions.hpp>
#include <unison/core/quaternion.hpp>
#include <unison/core/raw_value.hpp>

#include <catch2/catch_test_macros.hpp>

#include <bit>
#include <cstdint>
#include <limits>

namespace
{

std::uint32_t bitsOf(float value)
{
    return std::bit_cast<std::uint32_t>(value);
}

constexpr float kNegativeZero = -0.0F;
constexpr float kSmallestDenormal = std::numeric_limits<float>::denorm_min();

}

TEST_CASE("float3 stores three floats with no padding")
{
    STATIC_REQUIRE(sizeof(unison::Float3) == 3 * sizeof(float));
    STATIC_REQUIRE(alignof(unison::Float3) == alignof(float));
    STATIC_REQUIRE(unison::RawValue<unison::Float3>);
}

TEST_CASE("quaternion stores four floats with no padding")
{
    STATIC_REQUIRE(sizeof(unison::Quaternion) == 4 * sizeof(float));
    STATIC_REQUIRE(alignof(unison::Quaternion) == alignof(float));
    STATIC_REQUIRE(unison::RawValue<unison::Quaternion>);
}

TEST_CASE("float3 is zeroed by default")
{
    const unison::Float3 value{};

    REQUIRE(bitsOf(value.x) == 0U);
    REQUIRE(bitsOf(value.y) == 0U);
    REQUIRE(bitsOf(value.z) == 0U);
}

TEST_CASE("quaternion defaults to the identity rotation")
{
    const unison::Quaternion value{};

    REQUIRE(bitsOf(value.x) == 0U);
    REQUIRE(bitsOf(value.y) == 0U);
    REQUIRE(bitsOf(value.z) == 0U);
    REQUIRE(bitsOf(value.w) == bitsOf(1.0F));
}

TEST_CASE("the default quaternion converts to jolt's identity rotation")
{
    const JPH::Quat converted = unison::toJoltQuaternion(unison::Quaternion{});
    const JPH::Quat identity = JPH::Quat::sIdentity();

    REQUIRE(bitsOf(converted.GetX()) == bitsOf(identity.GetX()));
    REQUIRE(bitsOf(converted.GetY()) == bitsOf(identity.GetY()));
    REQUIRE(bitsOf(converted.GetZ()) == bitsOf(identity.GetZ()));
    REQUIRE(bitsOf(converted.GetW()) == bitsOf(identity.GetW()));
}

TEST_CASE("float3 round trips through a jolt vector bit for bit")
{
    const unison::Float3 original{1.5F, kNegativeZero, kSmallestDenormal};
    const unison::Float3 restored = unison::toFloat3(unison::toJoltVector(original));

    REQUIRE(bitsOf(restored.x) == bitsOf(original.x));
    REQUIRE(bitsOf(restored.y) == bitsOf(original.y));
    REQUIRE(bitsOf(restored.z) == bitsOf(original.z));
}

TEST_CASE("quaternion round trips through a jolt quaternion bit for bit")
{
    const unison::Quaternion original{kNegativeZero, -2.25F, kSmallestDenormal, 1.0F};
    const unison::Quaternion restored = unison::toQuaternion(unison::toJoltQuaternion(original));

    REQUIRE(bitsOf(restored.x) == bitsOf(original.x));
    REQUIRE(bitsOf(restored.y) == bitsOf(original.y));
    REQUIRE(bitsOf(restored.z) == bitsOf(original.z));
    REQUIRE(bitsOf(restored.w) == bitsOf(original.w));
}

TEST_CASE("a jolt vector round trips through float3 bit for bit")
{
    const JPH::Vec3 original{1.5F, kNegativeZero, kSmallestDenormal};
    const JPH::Vec3 restored = unison::toJoltVector(unison::toFloat3(original));

    REQUIRE(bitsOf(restored.GetX()) == bitsOf(original.GetX()));
    REQUIRE(bitsOf(restored.GetY()) == bitsOf(original.GetY()));
    REQUIRE(bitsOf(restored.GetZ()) == bitsOf(original.GetZ()));
}
