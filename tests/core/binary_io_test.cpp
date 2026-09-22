#include <unison/core/binary_reader.hpp>
#include <unison/core/binary_writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace
{

struct Pod
{
    std::uint16_t left;
    std::uint16_t right;

    bool operator==(const Pod& other) const = default;
};

constexpr std::array<std::uint8_t, 5> kPayload{1, 2, 3, 4, 5};

std::span<const std::byte> payloadBytes()
{
    return std::as_bytes(std::span{kPayload});
}

std::span<const std::byte> writtenBytes(std::span<const std::byte> storage, const unison::BinaryWriter& writer)
{
    return storage.first(writer.size());
}

}

TEST_CASE("binary round trip preserves a value")
{
    std::array<std::byte, 32> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE(writer.writeValue(std::uint32_t{0xDEADBEEF}));
    REQUIRE(writer.writeValue(Pod{7, 11}));

    unison::BinaryReader reader{writtenBytes(storage, writer)};

    REQUIRE(reader.readValue<std::uint32_t>() == 0xDEADBEEFU);
    REQUIRE(reader.readValue<Pod>() == Pod{7, 11});
    REQUIRE(reader.remaining() == 0U);
}

TEST_CASE("binary round trip preserves a byte span")
{
    std::array<std::byte, 32> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE(writer.writeBytes(payloadBytes()));

    unison::BinaryReader reader{writtenBytes(storage, writer)};
    const auto restored = reader.readBytes(kPayload.size());

    REQUIRE(restored.has_value());
    REQUIRE(std::equal(restored->begin(), restored->end(), payloadBytes().begin()));
    REQUIRE(reader.remaining() == 0U);
}

TEST_CASE("binary round trip preserves a string")
{
    std::array<std::byte, 32> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE(writer.writeString("unison"));

    unison::BinaryReader reader{writtenBytes(storage, writer)};

    REQUIRE(reader.readString() == "unison");
    REQUIRE(reader.remaining() == 0U);
}

TEST_CASE("binary round trip preserves an empty string")
{
    std::array<std::byte, 32> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE(writer.writeString(""));

    unison::BinaryReader reader{writtenBytes(storage, writer)};

    REQUIRE(reader.readString() == "");
    REQUIRE(reader.remaining() == 0U);
}

TEST_CASE("binary round trip preserves items written one after another")
{
    std::array<std::byte, 32> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE(writer.writeValue(std::uint8_t{42}));
    REQUIRE(writer.writeString("unison"));
    REQUIRE(writer.writeBytes(payloadBytes()));
    REQUIRE(writer.writeValue(Pod{1, 2}));

    unison::BinaryReader reader{writtenBytes(storage, writer)};

    REQUIRE(reader.readValue<std::uint8_t>() == 42U);
    REQUIRE(reader.readString() == "unison");
    REQUIRE(reader.readBytes(kPayload.size()).has_value());
    REQUIRE(reader.readValue<Pod>() == Pod{1, 2});
    REQUIRE(reader.remaining() == 0U);
}

TEST_CASE("binary reader fails on a truncated value and consumes nothing")
{
    std::array<std::byte, 32> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE(writer.writeValue(std::uint32_t{1}));

    unison::BinaryReader reader{writtenBytes(storage, writer).first(3)};

    REQUIRE_FALSE(reader.readValue<std::uint32_t>().has_value());
    REQUIRE(reader.remaining() == 3U);
}

TEST_CASE("binary reader fails on a string with a truncated body and consumes nothing")
{
    std::array<std::byte, 32> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE(writer.writeString("unison"));

    const std::span<const std::byte> truncated = writtenBytes(storage, writer).first(writer.size() - 1);
    unison::BinaryReader reader{truncated};

    REQUIRE_FALSE(reader.readString().has_value());
    REQUIRE(reader.remaining() == truncated.size());
}

TEST_CASE("binary reader fails past the end instead of returning stale bytes")
{
    std::array<std::byte, 32> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE(writer.writeValue(std::uint8_t{42}));

    unison::BinaryReader reader{writtenBytes(storage, writer)};

    REQUIRE(reader.readValue<std::uint8_t>() == 42U);
    REQUIRE_FALSE(reader.readValue<std::uint8_t>().has_value());
}

TEST_CASE("binary writer fails on a value that does not fit and writes nothing")
{
    std::array<std::byte, 3> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE_FALSE(writer.writeValue(std::uint32_t{1}));
    REQUIRE(writer.size() == 0U);
}

TEST_CASE("binary writer fails on a string that does not fit without writing its length")
{
    std::array<std::byte, 8> storage{};
    unison::BinaryWriter writer{storage};

    REQUIRE_FALSE(writer.writeString("unison"));
    REQUIRE(writer.size() == 0U);
}
