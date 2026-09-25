#pragma once

#include <unison/core/raw_value.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>

namespace unison
{

/// Reads back what BinaryWriter produced, bounds-checked against untrusted input: every read reports
/// failure in its return type and a failed read consumes nothing, so a truncated buffer stops the
/// decode instead of yielding stale bytes. Bytes and strings are views into the source buffer.
class BinaryReader
{
    static_assert(std::endian::native == std::endian::little, "Unison encodes little-endian, as x86-64 and arm64 are");

public:
    explicit BinaryReader(std::span<const std::byte> source) : source{source}
    {
    }

    template <RawValue T>
    [[nodiscard]] std::optional<T> readValue()
    {
        const std::optional<std::span<const std::byte>> bytes = readBytes(sizeof(T));

        if (!bytes.has_value())
        {
            return std::nullopt;
        }

        T value{};
        std::memcpy(&value, bytes->data(), sizeof(T));

        return value;
    }

    [[nodiscard]] std::optional<std::span<const std::byte>> readBytes(std::size_t count)
    {
        if (count > remaining())
        {
            return std::nullopt;
        }

        const std::span<const std::byte> bytes = source.subspan(position, count);
        position += count;

        return bytes;
    }

    [[nodiscard]] std::optional<std::string_view> readString()
    {
        const std::size_t start = position;
        const std::optional<std::uint32_t> length = readValue<std::uint32_t>();

        if (!length.has_value())
        {
            return std::nullopt;
        }

        const std::optional<std::span<const std::byte>> text = readBytes(*length);

        if (!text.has_value())
        {
            position = start;
            return std::nullopt;
        }

        return std::string_view{reinterpret_cast<const char*>(text->data()), text->size()};
    }

    [[nodiscard]] std::size_t remaining() const
    {
        return source.size() - position;
    }

private:
    std::span<const std::byte> source;
    std::size_t position = 0;
};

}
