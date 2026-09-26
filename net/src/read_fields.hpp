#pragma once

#include <unison/core/binary_reader.hpp>
#include <unison/core/raw_value.hpp>

#include <optional>

namespace unison::net
{

template <RawValue T>
bool readInto(BinaryReader& reader, T& value)
{
    const std::optional<T> read = reader.readValue<T>();

    if (!read.has_value())
    {
        return false;
    }

    value = *read;

    return true;
}

template <RawValue... Values>
bool readAll(BinaryReader& reader, Values&... values)
{
    return (readInto(reader, values) && ...);
}

}
