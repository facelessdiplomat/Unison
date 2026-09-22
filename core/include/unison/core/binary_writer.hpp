#pragma once

#include <unison/core/raw_value.hpp>

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>

namespace unison
{

/// Writes little-endian bytes into a caller-owned buffer, so the deterministic libraries never
/// allocate. Values go in as their object bytes, a string as a 32-bit length then its characters.
/// Every write reports whether it fit, and a write that does not fit leaves the buffer untouched.
class BinaryWriter
{
    static_assert(std::endian::native == std::endian::little, "Unison encodes little-endian and targets x64");

public:
    explicit BinaryWriter(std::span<std::byte> destination) : destination{destination}
    {
    }

    template <RawValue T>
    [[nodiscard]] bool writeValue(const T& value)
    {
        return writeBytes(std::as_bytes(std::span<const T, 1>{&value, 1}));
    }

    [[nodiscard]] bool writeBytes(std::span<const std::byte> bytes)
    {
        if (bytes.size() > remaining())
        {
            return false;
        }

        std::memcpy(destination.data() + position, bytes.data(), bytes.size());
        position += bytes.size();

        return true;
    }

    [[nodiscard]] bool writeString(std::string_view text)
    {
        assert(text.size() <= std::numeric_limits<std::uint32_t>::max());

        if (sizeof(std::uint32_t) + text.size() > remaining())
        {
            return false;
        }

        return writeValue(static_cast<std::uint32_t>(text.size())) && writeBytes(std::as_bytes(std::span{text}));
    }

    [[nodiscard]] std::size_t size() const
    {
        return position;
    }

    [[nodiscard]] std::size_t remaining() const
    {
        return destination.size() - position;
    }

private:
    std::span<std::byte> destination;
    std::size_t position = 0;
};

}
