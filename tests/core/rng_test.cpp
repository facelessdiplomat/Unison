#include <unison/core/raw_value.hpp>
#include <unison/core/rng.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace
{

constexpr std::uint64_t kGoldenSeed = 42;

constexpr std::array<std::uint32_t, 8> kGoldenUint32{
    0x15780B2EU,
    0x6104D986U,
    0xAE175332U,
    0xECB8AD47U,
    0xFDE6DC7FU,
    0xC50DA531U,
    0xB8215485U,
    0xD99A2743U,
};

constexpr std::array<std::uint32_t, 8> kGoldenFloatBits{
    0x3DABC058U,
    0x3EC209B2U,
    0x3F2E1753U,
    0x3F6CB8ADU,
    0x3F7DE6DCU,
    0x3F450DA5U,
    0x3F382154U,
    0x3F599A27U,
};

constexpr std::int32_t kRangeMinimum = -3;
constexpr std::int32_t kRangeMaximum = 5;
constexpr int kDrawCount = 10000;

std::uint32_t bitsOf(float value)
{
    return std::bit_cast<std::uint32_t>(value);
}

}

TEST_CASE("rng reproduces the reference xoshiro256 sequence for its seed")
{
    unison::Rng rng{kGoldenSeed};

    for (const std::uint32_t expected : kGoldenUint32)
    {
        REQUIRE(rng.nextUint32() == expected);
    }
}

TEST_CASE("rng reproduces the reference unit interval sequence for its seed")
{
    unison::Rng rng{kGoldenSeed};

    for (const std::uint32_t expected : kGoldenFloatBits)
    {
        REQUIRE(bitsOf(rng.nextFloat01()) == expected);
    }
}

TEST_CASE("rng unit interval draws stay inside the half open unit interval")
{
    unison::Rng rng{kGoldenSeed};
    bool allInside = true;

    for (int draw = 0; draw < kDrawCount; ++draw)
    {
        const float value = rng.nextFloat01();

        allInside = allInside && value >= 0.0F && value < 1.0F;
    }

    REQUIRE(allInside);
}

TEST_CASE("rng range draws stay inside their bounds")
{
    unison::Rng rng{kGoldenSeed};
    bool allInside = true;

    for (int draw = 0; draw < kDrawCount; ++draw)
    {
        const std::int32_t value = rng.nextInRange(kRangeMinimum, kRangeMaximum);

        allInside = allInside && value >= kRangeMinimum && value <= kRangeMaximum;
    }

    REQUIRE(allInside);
}

TEST_CASE("rng range draws reach both ends of the range")
{
    unison::Rng rng{kGoldenSeed};
    std::array<bool, 9> seen{};

    for (int draw = 0; draw < kDrawCount; ++draw)
    {
        const std::int32_t value = rng.nextInRange(kRangeMinimum, kRangeMaximum);

        seen[static_cast<std::size_t>(value - kRangeMinimum)] = true;
    }

    REQUIRE(std::all_of(seen.begin(), seen.end(), [](bool value) { return value; }));
}

TEST_CASE("rng range of a single value always returns that value")
{
    unison::Rng rng{kGoldenSeed};

    REQUIRE(rng.nextInRange(4, 4) == 4);
    REQUIRE(rng.nextInRange(4, 4) == 4);
}

TEST_CASE("rng state is a raw value that a copy carries on from")
{
    STATIC_REQUIRE(std::is_trivially_copyable_v<unison::Rng>);
    STATIC_REQUIRE(unison::RawValue<unison::Rng>);

    unison::Rng original{kGoldenSeed};
    static_cast<void>(original.nextUint32());

    unison::Rng copy = original;

    const std::uint32_t fromCopy = copy.nextUint32();
    const std::uint32_t fromOriginal = original.nextUint32();

    REQUIRE(fromCopy == fromOriginal);
}
