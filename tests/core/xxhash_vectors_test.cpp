#include <catch2/catch_test_macros.hpp>

#include <xxhash.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint64_t kSanitySeed = 2654435761ULL;
constexpr std::uint64_t kSanityMultiplier = 11400714785074694797ULL;

std::vector<std::uint8_t> makeSanityBuffer(std::size_t length)
{
    std::vector<std::uint8_t> buffer(length);
    std::uint64_t byteGenerator = kSanitySeed;

    for (std::size_t index = 0; index < length; ++index)
    {
        buffer[index] = static_cast<std::uint8_t>(byteGenerator >> 56);
        byteGenerator *= kSanityMultiplier;
    }

    return buffer;
}

struct KnownAnswer
{
    std::size_t length;
    std::uint64_t seed;
    std::uint64_t hash;
};

constexpr std::uint64_t kSeededSeed = 0x9E3779B185EBCA8DULL;

constexpr std::array<KnownAnswer, 12> kKnownAnswers{{
    {0, 0, 0x2D06800538D394C2ULL},
    {0, kSeededSeed, 0xA8A6B918B2F0364AULL},
    {1, 0, 0xC44BDFF4074EECDBULL},
    {1, kSeededSeed, 0x032BE332DD766EF8ULL},
    {4, 0, 0xE5DC74BC51848A51ULL},
    {4, kSeededSeed, 0xAA2E7ECCB0C8F747ULL},
    {12, 0, 0xA713DAF0DFBB77E7ULL},
    {12, kSeededSeed, 0xE7303E1B2336DE0EULL},
    {64, 0, 0x9CB48487720EC49DULL},
    {64, kSeededSeed, 0x4FE8895DB9B8C077ULL},
    {240, 0, 0x81C3C2B67F568CCFULL},
    {2048, 0, 0xDD59E2C3A5F038E0ULL},
}};

}

TEST_CASE("xxh3 reproduces the xxhash sanity vectors")
{
    for (const KnownAnswer& answer : kKnownAnswers)
    {
        CAPTURE(answer.length, answer.seed);

        const std::vector<std::uint8_t> buffer = makeSanityBuffer(answer.length);

        REQUIRE(XXH3_64bits_withSeed(buffer.data(), buffer.size(), answer.seed) == answer.hash);

        if (answer.seed == 0)
        {
            REQUIRE(XXH3_64bits(buffer.data(), buffer.size()) == answer.hash);
        }
    }
}
