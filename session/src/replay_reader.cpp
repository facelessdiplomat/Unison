#include <unison/session/replay_reader.hpp>

#include "replay_encoding.hpp"

#include <optional>

namespace unison::session
{

namespace
{

tl::unexpected<Error> endsEarly()
{
    return tl::unexpected{Error{ErrorCode::TruncatedInput, "the replay ends inside its header or a record"}};
}

tl::unexpected<Error> malformed(std::string_view reason)
{
    return tl::unexpected{Error{ErrorCode::MalformedReplay, reason}};
}

tl::expected<void, Error> readPreamble(BinaryReader& reader)
{
    const std::optional<std::uint32_t> magic = reader.readValue<std::uint32_t>();

    if (!magic.has_value())
    {
        return endsEarly();
    }

    if (*magic != kReplayMagic)
    {
        return malformed("the bytes do not open with the replay magic");
    }

    const std::optional<std::uint16_t> version = reader.readValue<std::uint16_t>();

    if (!version.has_value())
    {
        return endsEarly();
    }

    if (*version != kReplayVersion)
    {
        return tl::unexpected{
            Error{ErrorCode::UnsupportedReplayVersion, "the replay was written in another version of the format"}};
    }

    return {};
}

tl::expected<net::SessionConfig, Error> readConfig(BinaryReader& reader)
{
    const std::optional<net::SessionConfig> config = net::readSessionConfig(reader);

    if (!config.has_value())
    {
        return endsEarly();
    }

    if (!isPlayable(*config))
    {
        return malformed("the replay's config is not one a session can play");
    }

    return *config;
}

}

tl::expected<ReplayReader, Error> ReplayReader::open(std::span<const std::byte> bytes)
{
    BinaryReader reader{bytes};

    return readPreamble(reader)
        .and_then([&reader] { return readConfig(reader); })
        .map([&reader](const net::SessionConfig& config) { return ReplayReader{reader, config}; });
}

ReplayReader::ReplayReader(const BinaryReader& reader, const net::SessionConfig& config)
    : reader{reader}, recordedConfig{config}
{
}

const net::SessionConfig& ReplayReader::config() const
{
    return recordedConfig;
}

bool ReplayReader::isAtEnd() const
{
    return reader.remaining() == 0;
}

tl::expected<ReplayRecord, Error> ReplayReader::next()
{
    const std::optional<ReplayRecordKind> kind = reader.readValue<ReplayRecordKind>();

    if (!kind.has_value())
    {
        return endsEarly();
    }

    switch (*kind)
    {
        case ReplayRecordKind::Frame:
            return nextFrame();
        case ReplayRecordKind::Checksum:
            return nextChecksum();
    }

    return malformed("the replay holds a record of no kind the format knows");
}

tl::expected<ReplayRecord, Error> ReplayReader::nextFrame()
{
    const std::optional<std::uint32_t> frameNumber = reader.readValue<std::uint32_t>();

    if (!frameNumber.has_value())
    {
        return endsEarly();
    }

    ReplayFrame frame{*frameNumber, {}};

    for (std::size_t slot = 0; slot < recordedConfig.slotCount; ++slot)
    {
        const std::optional<sim::InputFlags> flags = reader.readValue<sim::InputFlags>();
        const std::optional<std::span<const std::byte>> input =
            flags.has_value() ? reader.readBytes(recordedConfig.inputSize) : std::nullopt;

        if (!input.has_value())
        {
            return endsEarly();
        }

        frame.inputs.setBytes(slot, *input, *flags);
    }

    return frame;
}

tl::expected<ReplayRecord, Error> ReplayReader::nextChecksum()
{
    const std::optional<std::uint32_t> frameNumber = reader.readValue<std::uint32_t>();
    const std::optional<std::uint64_t> checksum =
        frameNumber.has_value() ? reader.readValue<std::uint64_t>() : std::nullopt;

    if (!checksum.has_value())
    {
        return endsEarly();
    }

    return VerifiedChecksum{*frameNumber, *checksum};
}

}
