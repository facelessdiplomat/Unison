#pragma once

#include <unison/core/error.hpp>
#include <unison/net/protocol.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace unison::net
{

/// Writes a message into the buffer as its type tag followed by its fields, little-endian. Returns how many
/// bytes it wrote, or an error when the buffer is too small for the message.
[[nodiscard]] tl::expected<std::size_t, Error> encode(const Message& message, std::span<std::byte> buffer);

/// Reads back a message that came over the wire. Bytes that end too soon, an unknown type or value, a
/// chunk numbered past its count or bytes left over are rejected. The spans of the message view the bytes.
[[nodiscard]] tl::expected<Message, Error> decode(std::span<const std::byte> bytes);

/// The most frames of this many slots of this input size one `Confirmed` carries within `kMaxDatagramSize`.
[[nodiscard]] std::uint32_t confirmedFramesPerDatagram(std::uint8_t slotCount, std::uint8_t inputSize);

}
