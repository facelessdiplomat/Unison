#include <unison/session/replay_writer.hpp>

#include "replay_encoding.hpp"

#include <unison/core/binary_writer.hpp>
#include <unison/core/contract.hpp>

#include <array>

namespace unison::session
{

namespace
{

constexpr std::size_t kLargestRecord =
    sizeof(ReplayRecordKind) + sizeof(std::uint32_t) + sim::kMaxSlots * (sizeof(sim::InputFlags) + sim::kMaxInputSize);

template <typename Encode>
void append(std::vector<std::byte>& encoded, Encode encode)
{
    std::array<std::byte, kLargestRecord> room{};
    BinaryWriter writer{room};
    const bool isWritten = encode(writer);

    UNISON_VERIFY(isWritten);

    const std::span<const std::byte> written = std::span{room}.first(writer.size());
    encoded.insert(encoded.end(), written.begin(), written.end());
}

}

ReplayWriter::ReplayWriter(const net::SessionConfig& config) : config{config}
{
    UNISON_VERIFY(isPlayable(config));

    append(encoded,
           [&config](BinaryWriter& writer)
           {
               return writer.writeValue(kReplayMagic) && writer.writeValue(kReplayVersion) &&
                      net::writeSessionConfig(writer, config);
           });
}

void ReplayWriter::writeFrame(std::uint32_t frameNumber, const sim::FrameInputs& inputs)
{
    append(encoded,
           [this, frameNumber, &inputs](BinaryWriter& writer)
           {
               bool isWritten = writer.writeValue(ReplayRecordKind::Frame) && writer.writeValue(frameNumber);

               for (std::size_t slot = 0; isWritten && slot < config.slotCount; ++slot)
               {
                   isWritten = writer.writeValue(inputs.flagsAt(slot)) &&
                               writer.writeBytes(inputs.bytesAt(slot).first(config.inputSize));
               }

               return isWritten;
           });
}

void ReplayWriter::writeChecksum(const VerifiedChecksum& checksum)
{
    append(encoded,
           [&checksum](BinaryWriter& writer)
           {
               return writer.writeValue(ReplayRecordKind::Checksum) && writer.writeValue(checksum.frameNumber) &&
                      writer.writeValue(checksum.checksum);
           });
}

void ReplayWriter::frameVerified(const VerifiedFrame& frame)
{
    writeFrame(frame.frameNumber, frame.inputs);

    if (frame.checksum.has_value())
    {
        writeChecksum(VerifiedChecksum{frame.frameNumber, *frame.checksum});
    }
}

std::span<const std::byte> ReplayWriter::bytes() const
{
    return encoded;
}

}
