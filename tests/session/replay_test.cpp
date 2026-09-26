#include <unison/session/replay_reader.hpp>
#include <unison/session/replay_writer.hpp>

#include <unison/core/binary_writer.hpp>
#include <unison/core/error.hpp>
#include <unison/net/session_config.hpp>

#include <support/fatal_handler_probe.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace
{

constexpr std::uint8_t kSlotCount = 2;
constexpr std::uint8_t kInputSize = 3;
constexpr std::size_t kHeaderRoom = 64;

unison::net::SessionConfig replayConfig()
{
    unison::net::SessionConfig config;
    config.slotCount = kSlotCount;
    config.inputSize = kInputSize;
    config.checksumInterval = 1;
    config.seed = 7;
    config.assetHash = 0x1122334455667788ULL;
    config.pipelineHash = 0x99AABBCCDDEEFF00ULL;

    return config;
}

unison::sim::FrameInputs inputsOf(std::uint8_t first, std::uint8_t second)
{
    const std::array<std::byte, kInputSize> firstInput{std::byte{first}, std::byte{0x10}, std::byte{0x20}};
    const std::array<std::byte, kInputSize> secondInput{std::byte{second}, std::byte{0x30}, std::byte{0x40}};
    unison::sim::FrameInputs inputs;
    inputs.setBytes(0, firstInput, unison::sim::InputFlags::Present);
    inputs.setBytes(1, secondInput, unison::sim::InputFlags::Dropped);

    return inputs;
}

bool hasSameInputs(const unison::sim::FrameInputs& left, const unison::sim::FrameInputs& right)
{
    for (std::size_t slot = 0; slot < kSlotCount; ++slot)
    {
        if (left.flagsAt(slot) != right.flagsAt(slot) ||
            !std::ranges::equal(left.bytesAt(slot).first(kInputSize), right.bytesAt(slot).first(kInputSize)))
        {
            return false;
        }
    }

    return true;
}

bool isFrame(const unison::session::ReplayRecord& record,
             std::uint32_t frameNumber,
             const unison::sim::FrameInputs& inputs)
{
    const auto* frame = std::get_if<unison::session::ReplayFrame>(&record);

    return frame != nullptr && frame->frameNumber == frameNumber && hasSameInputs(frame->inputs, inputs);
}

bool isChecksum(const unison::session::ReplayRecord& record, const unison::session::VerifiedChecksum& expected)
{
    const auto* checksum = std::get_if<unison::session::VerifiedChecksum>(&record);

    return checksum != nullptr && checksum->frameNumber == expected.frameNumber &&
           checksum->checksum == expected.checksum;
}

std::vector<unison::session::ReplayRecord> recordsOf(unison::session::ReplayReader& reader)
{
    std::vector<unison::session::ReplayRecord> records;

    while (!reader.isAtEnd())
    {
        const tl::expected<unison::session::ReplayRecord, unison::Error> record = reader.next();

        if (!record.has_value())
        {
            break;
        }

        records.push_back(*record);
    }

    return records;
}

std::vector<std::byte> replayBytes()
{
    unison::session::ReplayWriter writer{replayConfig()};
    writer.writeFrame(1, inputsOf(1, 2));

    return {writer.bytes().begin(), writer.bytes().end()};
}

std::vector<std::byte> headerOf(std::uint32_t magic, std::uint16_t version, const unison::net::SessionConfig& config)
{
    std::array<std::byte, kHeaderRoom> room{};
    unison::BinaryWriter writer{room};
    const bool isWritten =
        writer.writeValue(magic) && writer.writeValue(version) && unison::net::writeSessionConfig(writer, config);

    return isWritten ? std::vector<std::byte>{room.begin(), room.begin() + static_cast<std::ptrdiff_t>(writer.size())}
                     : std::vector<std::byte>{};
}

}

TEST_CASE("a replay reads back as the config and the records it was written with")
{
    const unison::session::VerifiedChecksum checksum{1, 0xABCDEF0123456789ULL};
    unison::session::ReplayWriter writer{replayConfig()};
    writer.writeFrame(1, inputsOf(1, 2));
    writer.writeChecksum(checksum);
    writer.writeFrame(2, inputsOf(3, 4));

    tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(writer.bytes());
    REQUIRE(reader.has_value());
    const std::vector<unison::session::ReplayRecord> records = recordsOf(*reader);

    REQUIRE(unison::net::hashOf(reader->config()) == unison::net::hashOf(replayConfig()));
    REQUIRE(records.size() == 3U);
    REQUIRE(isFrame(records[0], 1, inputsOf(1, 2)));
    REQUIRE(isChecksum(records[1], checksum));
    REQUIRE(isFrame(records[2], 2, inputsOf(3, 4)));
}

TEST_CASE("a replay written as a session verifies frames holds each frame and the checksum taken of it")
{
    const unison::session::VerifiedChecksum checksum{2, 0x0123456789ABCDEFULL};
    unison::session::ReplayWriter writer{replayConfig()};
    unison::session::IVerifiedFrameReceiver& receiver = writer;
    receiver.frameVerified(1, inputsOf(1, 2), std::nullopt);
    receiver.frameVerified(2, inputsOf(3, 4), checksum.checksum);

    tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(writer.bytes());
    REQUIRE(reader.has_value());
    const std::vector<unison::session::ReplayRecord> records = recordsOf(*reader);

    REQUIRE(records.size() == 3U);
    REQUIRE(isFrame(records[0], 1, inputsOf(1, 2)));
    REQUIRE(isFrame(records[1], 2, inputsOf(3, 4)));
    REQUIRE(isChecksum(records[2], checksum));
}

TEST_CASE("bytes that do not open with the replay magic are refused")
{
    const std::vector<std::byte> bytes =
        headerOf(unison::session::kReplayMagic + 1U, unison::session::kReplayVersion, replayConfig());

    const tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(bytes);

    REQUIRE_FALSE(reader.has_value());
    REQUIRE(reader.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay of another version of the format is refused")
{
    const std::vector<std::byte> bytes = headerOf(unison::session::kReplayMagic,
                                                  static_cast<std::uint16_t>(unison::session::kReplayVersion + 1U),
                                                  replayConfig());

    const tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(bytes);

    REQUIRE_FALSE(reader.has_value());
    REQUIRE(reader.error().code() == unison::ErrorCode::UnsupportedReplayVersion);
}

TEST_CASE("a replay whose config has more slots than a frame holds is refused")
{
    unison::net::SessionConfig config = replayConfig();
    config.slotCount = unison::sim::kMaxSlots + 1U;
    const std::vector<std::byte> bytes =
        headerOf(unison::session::kReplayMagic, unison::session::kReplayVersion, config);

    const tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(bytes);

    REQUIRE_FALSE(reader.has_value());
    REQUIRE(reader.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay whose config has larger inputs than a slot holds is refused")
{
    unison::net::SessionConfig config = replayConfig();
    config.inputSize = unison::sim::kMaxInputSize + 1U;
    const std::vector<std::byte> bytes =
        headerOf(unison::session::kReplayMagic, unison::session::kReplayVersion, config);

    const tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(bytes);

    REQUIRE_FALSE(reader.has_value());
    REQUIRE(reader.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay whose config seats no player is refused")
{
    unison::net::SessionConfig config = replayConfig();
    config.slotCount = 0;
    const std::vector<std::byte> bytes =
        headerOf(unison::session::kReplayMagic, unison::session::kReplayVersion, config);

    const tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(bytes);

    REQUIRE_FALSE(reader.has_value());
    REQUIRE(reader.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay whose config takes checksums on no interval is refused")
{
    unison::net::SessionConfig config = replayConfig();
    config.checksumInterval = 0;
    const std::vector<std::byte> bytes =
        headerOf(unison::session::kReplayMagic, unison::session::kReplayVersion, config);

    const tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(bytes);

    REQUIRE_FALSE(reader.has_value());
    REQUIRE(reader.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay whose config ticks no times a second is refused")
{
    unison::net::SessionConfig config = replayConfig();
    config.tickRate = 0;
    const std::vector<std::byte> bytes =
        headerOf(unison::session::kReplayMagic, unison::session::kReplayVersion, config);

    const tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(bytes);

    REQUIRE_FALSE(reader.has_value());
    REQUIRE(reader.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay that ends inside its header is refused")
{
    const std::vector<std::byte> bytes = replayBytes();

    const tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(std::span{bytes}.first(10));

    REQUIRE_FALSE(reader.has_value());
    REQUIRE(reader.error().code() == unison::ErrorCode::TruncatedInput);
}

TEST_CASE("a replay that ends inside a record is refused")
{
    const std::vector<std::byte> bytes = replayBytes();
    tl::expected<unison::session::ReplayReader, unison::Error> reader =
        unison::session::ReplayReader::open(std::span{bytes}.first(bytes.size() - 1));
    REQUIRE(reader.has_value());

    const tl::expected<unison::session::ReplayRecord, unison::Error> record = reader->next();

    REQUIRE_FALSE(record.has_value());
    REQUIRE(record.error().code() == unison::ErrorCode::TruncatedInput);
}

TEST_CASE("a record of no kind the replay format knows is refused")
{
    std::vector<std::byte> bytes =
        headerOf(unison::session::kReplayMagic, unison::session::kReplayVersion, replayConfig());
    bytes.push_back(std::byte{0x7F});
    tl::expected<unison::session::ReplayReader, unison::Error> reader = unison::session::ReplayReader::open(bytes);
    REQUIRE(reader.has_value());

    const tl::expected<unison::session::ReplayRecord, unison::Error> record = reader->next();

    REQUIRE_FALSE(record.has_value());
    REQUIRE(record.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay writer for more slots than a frame holds breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::net::SessionConfig config = replayConfig();
    config.slotCount = unison::sim::kMaxSlots + 1U;

    const unison::session::ReplayWriter writer{config};

    REQUIRE(probe.failureCount() == 1U);
}
