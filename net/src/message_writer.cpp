#include <unison/net/message_codec.hpp>

#include <unison/core/binary_writer.hpp>
#include <unison/core/contract.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <variant>

namespace unison::net
{

namespace
{

bool writeFields(BinaryWriter& writer, const SessionConfig& config)
{
    return writer.writeValue(config.tickRate) && writer.writeValue(config.slotCount) &&
           writer.writeValue(config.inputSize) && writer.writeValue(config.maxPrediction) &&
           writer.writeValue(config.checksumInterval) && writer.writeValue(config.seed) &&
           writer.writeValue(config.assetHash) && writer.writeValue(config.pipelineHash) &&
           writer.writeValue(config.buildId);
}

bool writeFields(BinaryWriter& writer, const Hello& hello)
{
    return writer.writeValue(hello.protocolVersion) && writeFields(writer, hello.config) &&
           writer.writeValue(hello.role) && writer.writeValue(hello.reconnectToken);
}

bool writeFields(BinaryWriter& writer, const Welcome& welcome)
{
    return writer.writeValue(welcome.slot) && writeFields(writer, welcome.config) &&
           writer.writeValue(welcome.startFrame) && writer.writeValue(welcome.confirmedFrame) &&
           writer.writeValue(welcome.reconnectToken);
}

bool writeFields(BinaryWriter& writer, const Input& input)
{
    UNISON_VERIFY(input.inputs.size() == std::size_t{input.inputSize} * input.frameCount);

    return writer.writeValue(input.firstFrame) && writer.writeValue(input.inputSize) &&
           writer.writeValue(input.frameCount) && writer.writeBytes(input.inputs);
}

bool writeFields(BinaryWriter& writer, const Confirmed& confirmed)
{
    UNISON_VERIFY(confirmed.slots.size() ==
                  confirmedFrameSize(confirmed.slotCount, confirmed.inputSize) * confirmed.frameCount);

    return writer.writeValue(confirmed.firstFrame) && writer.writeValue(confirmed.slotCount) &&
           writer.writeValue(confirmed.inputSize) && writer.writeValue(confirmed.frameCount) &&
           writer.writeBytes(confirmed.slots);
}

bool writeFields(BinaryWriter& writer, const Checksum& checksum)
{
    return writer.writeValue(checksum.frame) && writer.writeValue(checksum.checksum);
}

bool writeFields(BinaryWriter& writer, const Desync& desync)
{
    return writer.writeValue(desync.frame) && writer.writeValue(desync.minoritySlots);
}

bool writeFields(BinaryWriter& writer, const SnapshotRequest& request)
{
    return writer.writeValue(request.frame);
}

bool writeFields(BinaryWriter& writer, const SnapshotChunk& chunk)
{
    return writer.writeValue(chunk.frame) && writer.writeValue(chunk.chunkIndex) &&
           writer.writeValue(chunk.chunkCount) && writer.writeValue(static_cast<std::uint32_t>(chunk.bytes.size())) &&
           writer.writeBytes(chunk.bytes);
}

bool writeFields(BinaryWriter& writer, const Ping& ping)
{
    return writer.writeValue(ping.sentAt);
}

bool writeFields(BinaryWriter& writer, const Pong& pong)
{
    return writer.writeValue(pong.pingSentAt) && writer.writeValue(pong.confirmedFrame) &&
           writer.writeValue(pong.dueFrame);
}

bool writeFields(BinaryWriter& writer, const Leave& leave)
{
    return writer.writeValue(leave.reason);
}

bool writeFields(BinaryWriter& writer, const Kick& kick)
{
    return writer.writeValue(kick.reason);
}

}

tl::expected<std::size_t, Error> encode(const Message& message, std::span<std::byte> buffer)
{
    BinaryWriter writer{buffer};
    const auto tag = static_cast<std::uint8_t>(message.index() + 1);

    const bool written = writer.writeValue(tag) &&
                         std::visit([&writer](const auto& fields) { return writeFields(writer, fields); }, message);

    if (!written)
    {
        return tl::unexpected{Error{ErrorCode::BufferTooSmall, "the message does not fit the buffer"}};
    }

    return writer.size();
}

std::uint32_t confirmedFramesPerDatagram(std::uint8_t slotCount, std::uint8_t inputSize)
{
    constexpr std::size_t kHeaderSize = sizeof(std::uint8_t) + sizeof(Confirmed::firstFrame) +
                                        sizeof(Confirmed::slotCount) + sizeof(Confirmed::inputSize) +
                                        sizeof(Confirmed::frameCount);
    constexpr std::size_t kMostFrames = std::numeric_limits<decltype(Confirmed::frameCount)>::max();
    const std::size_t frameSize = confirmedFrameSize(slotCount, inputSize);

    if (frameSize == 0)
    {
        return kMostFrames;
    }

    return static_cast<std::uint32_t>(std::min(kMostFrames, (kMaxDatagramSize - kHeaderSize) / frameSize));
}

}
