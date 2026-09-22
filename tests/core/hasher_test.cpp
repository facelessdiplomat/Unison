#include <unison/core/hasher.hpp>

#include <catch2/catch_test_macros.hpp>

#include <xxhash.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace
{

constexpr std::array<std::uint8_t, 9> kBuffer{1, 2, 3, 4, 5, 6, 7, 8, 9};

std::span<const std::byte> bufferBytes(std::size_t offset, std::size_t count)
{
    return std::as_bytes(std::span<const std::uint8_t>{kBuffer.data() + offset, count});
}

struct Sample
{
    std::uint32_t left;
    std::uint32_t right;
};

}

TEST_CASE("an untouched hasher matches the one-shot XXH3 of no bytes")
{
    const unison::Hasher hasher;

    REQUIRE(hasher.finish() == XXH3_64bits(kBuffer.data(), 0));
}

TEST_CASE("hasher digest depends on the bytes, not on how they were split")
{
    unison::Hasher inOnePiece;
    inOnePiece.add(bufferBytes(0, 9));

    unison::Hasher inThreePieces;
    inThreePieces.add(bufferBytes(0, 2));
    inThreePieces.add(bufferBytes(2, 3));
    inThreePieces.add(bufferBytes(5, 4));

    REQUIRE(inOnePiece.finish() == inThreePieces.finish());
}

TEST_CASE("hasher digest depends on the order bytes were added")
{
    unison::Hasher forward;
    forward.add(bufferBytes(0, 4));
    forward.add(bufferBytes(4, 5));

    unison::Hasher reversed;
    reversed.add(bufferBytes(4, 5));
    reversed.add(bufferBytes(0, 4));

    REQUIRE(forward.finish() != reversed.finish());
}

TEST_CASE("hasher matches the one-shot XXH3 result over the same bytes")
{
    unison::Hasher hasher;

    hasher.add(bufferBytes(0, 4));
    hasher.add(bufferBytes(4, 5));

    REQUIRE(hasher.finish() == XXH3_64bits(kBuffer.data(), kBuffer.size()));
}

TEST_CASE("hasher hashes a trivially copyable value as its object bytes")
{
    constexpr Sample sample{7, 11};

    unison::Hasher hasher;
    hasher.add(sample);

    REQUIRE(hasher.finish() == XXH3_64bits(&sample, sizeof(Sample)));
}

TEST_CASE("hasher ignores empty additions")
{
    unison::Hasher withEmptyAdditions;
    withEmptyAdditions.add(std::span<const std::byte>{});
    withEmptyAdditions.add(bufferBytes(0, 9));
    withEmptyAdditions.add(std::span<const std::byte>{});

    unison::Hasher withoutEmptyAdditions;
    withoutEmptyAdditions.add(bufferBytes(0, 9));

    REQUIRE(withEmptyAdditions.finish() == withoutEmptyAdditions.finish());
}

TEST_CASE("hasher hashes a byte span of static extent as its bytes")
{
    unison::Hasher hasher;

    hasher.add(std::as_bytes(std::span{kBuffer}));

    REQUIRE(hasher.finish() == XXH3_64bits(kBuffer.data(), kBuffer.size()));
}
