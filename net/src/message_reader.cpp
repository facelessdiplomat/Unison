#include <unison/net/message_codec.hpp>

#include <unison/core/binary_reader.hpp>
#include <unison/core/raw_value.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

namespace unison::net
{

namespace
{

tl::unexpected<Error> truncated()
{
    return tl::unexpected{Error{ErrorCode::TruncatedInput, "the message ends before its fields do"}};
}

tl::unexpected<Error> malformed()
{
    return tl::unexpected{Error{ErrorCode::MalformedMessage, "the bytes are not a message the protocol knows"}};
}

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

bool readBytesInto(BinaryReader& reader, std::size_t count, std::span<const std::byte>& bytes)
{
    const std::optional<std::span<const std::byte>> read = reader.readBytes(count);

    if (!read.has_value())
    {
        return false;
    }

    bytes = *read;

    return true;
}

bool isKnown(Role role)
{
    return role <= Role::Spectator;
}

bool isKnown(LeaveReason reason)
{
    return reason <= LeaveReason::RoomFull;
}

bool readConfig(BinaryReader& reader, SessionConfig& config)
{
    return readAll(reader,
                   config.tickRate,
                   config.slotCount,
                   config.inputSize,
                   config.maxPrediction,
                   config.checksumInterval,
                   config.seed,
                   config.assetHash,
                   config.pipelineHash,
                   config.buildId);
}

template <typename T>
tl::expected<T, Error> readFields(BinaryReader& reader);

template <>
tl::expected<Hello, Error> readFields<Hello>(BinaryReader& reader)
{
    Hello hello;

    if (!readAll(reader, hello.protocolVersion) || !readConfig(reader, hello.config) ||
        !readAll(reader, hello.role, hello.reconnectToken))
    {
        return truncated();
    }

    if (!isKnown(hello.role))
    {
        return malformed();
    }

    return hello;
}

template <>
tl::expected<Welcome, Error> readFields<Welcome>(BinaryReader& reader)
{
    Welcome welcome;

    if (!readAll(reader, welcome.slot) || !readConfig(reader, welcome.config) ||
        !readAll(reader, welcome.startFrame, welcome.confirmedFrame, welcome.reconnectToken))
    {
        return truncated();
    }

    return welcome;
}

template <>
tl::expected<Input, Error> readFields<Input>(BinaryReader& reader)
{
    Input input;

    if (!readAll(reader, input.firstFrame, input.inputSize, input.frameCount) ||
        !readBytesInto(reader, std::size_t{input.inputSize} * input.frameCount, input.inputs))
    {
        return truncated();
    }

    return input;
}

template <>
tl::expected<Confirmed, Error> readFields<Confirmed>(BinaryReader& reader)
{
    Confirmed confirmed;

    if (!readAll(reader, confirmed.firstFrame, confirmed.slotCount, confirmed.inputSize, confirmed.frameCount) ||
        !readBytesInto(reader,
                       confirmedFrameSize(confirmed.slotCount, confirmed.inputSize) * confirmed.frameCount,
                       confirmed.slots))
    {
        return truncated();
    }

    return confirmed;
}

template <>
tl::expected<Checksum, Error> readFields<Checksum>(BinaryReader& reader)
{
    Checksum checksum;

    if (!readAll(reader, checksum.frame, checksum.checksum))
    {
        return truncated();
    }

    return checksum;
}

template <>
tl::expected<Desync, Error> readFields<Desync>(BinaryReader& reader)
{
    Desync desync;

    if (!readAll(reader, desync.frame, desync.minoritySlots))
    {
        return truncated();
    }

    return desync;
}

template <>
tl::expected<SnapshotRequest, Error> readFields<SnapshotRequest>(BinaryReader& reader)
{
    SnapshotRequest request;

    if (!readAll(reader, request.frame))
    {
        return truncated();
    }

    return request;
}

template <>
tl::expected<SnapshotChunk, Error> readFields<SnapshotChunk>(BinaryReader& reader)
{
    SnapshotChunk chunk;
    std::uint32_t length = 0;

    if (!readAll(reader, chunk.frame, chunk.chunkIndex, chunk.chunkCount, length) ||
        !readBytesInto(reader, length, chunk.bytes))
    {
        return truncated();
    }

    if (chunk.chunkIndex >= chunk.chunkCount)
    {
        return malformed();
    }

    return chunk;
}

template <>
tl::expected<Ping, Error> readFields<Ping>(BinaryReader& reader)
{
    Ping ping;

    if (!readAll(reader, ping.sentAt))
    {
        return truncated();
    }

    return ping;
}

template <>
tl::expected<Pong, Error> readFields<Pong>(BinaryReader& reader)
{
    Pong pong;

    if (!readAll(reader, pong.pingSentAt, pong.confirmedFrame, pong.dueFrame))
    {
        return truncated();
    }

    return pong;
}

template <>
tl::expected<Leave, Error> readFields<Leave>(BinaryReader& reader)
{
    Leave leave;

    if (!readAll(reader, leave.reason))
    {
        return truncated();
    }

    if (!isKnown(leave.reason))
    {
        return malformed();
    }

    return leave;
}

template <>
tl::expected<Kick, Error> readFields<Kick>(BinaryReader& reader)
{
    Kick kick;

    if (!readAll(reader, kick.reason))
    {
        return truncated();
    }

    if (!isKnown(kick.reason))
    {
        return malformed();
    }

    return kick;
}

template <typename T>
tl::expected<Message, Error> readMessage(BinaryReader& reader)
{
    tl::expected<T, Error> fields = readFields<T>(reader);

    if (!fields.has_value())
    {
        return tl::unexpected{fields.error()};
    }

    return Message{*fields};
}

using MessageReader = tl::expected<Message, Error> (*)(BinaryReader&);

template <std::size_t... Index>
constexpr std::array<MessageReader, sizeof...(Index)> readersInTagOrder(std::index_sequence<Index...>)
{
    return {&readMessage<std::variant_alternative_t<Index, Message>>...};
}

constexpr auto kReaders = readersInTagOrder(std::make_index_sequence<std::variant_size_v<Message>>{});

}

tl::expected<Message, Error> decode(std::span<const std::byte> bytes)
{
    BinaryReader reader{bytes};
    const std::optional<std::uint8_t> tag = reader.readValue<std::uint8_t>();

    if (!tag.has_value())
    {
        return truncated();
    }

    if (*tag == 0 || *tag > kReaders.size())
    {
        return malformed();
    }

    tl::expected<Message, Error> message = kReaders[*tag - 1U](reader);

    if (message.has_value() && reader.remaining() != 0)
    {
        return malformed();
    }

    return message;
}

}
