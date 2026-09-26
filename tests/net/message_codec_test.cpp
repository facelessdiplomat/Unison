#include <unison/net/message_codec.hpp>

#include <unison/core/error.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

namespace
{

constexpr std::size_t kRoomForAnyMessage = 1200;

std::vector<std::byte> encoded(const unison::net::Message& message)
{
    std::vector<std::byte> buffer(kRoomForAnyMessage);
    const auto written = unison::net::encode(message, buffer);

    REQUIRE(written.has_value());
    buffer.resize(*written);

    return buffer;
}

template <typename T>
T decodedAs(const std::vector<std::byte>& bytes)
{
    const auto decoded = unison::net::decode(bytes);

    REQUIRE(decoded.has_value());
    REQUIRE(std::holds_alternative<T>(*decoded));

    return std::get<T>(*decoded);
}

unison::ErrorCode failureOf(std::span<const std::byte> bytes)
{
    const auto decoded = unison::net::decode(bytes);

    REQUIRE_FALSE(decoded.has_value());

    return decoded.error().code();
}

unison::net::SessionConfig sampleConfig()
{
    unison::net::SessionConfig config;
    config.tickRate = 30;
    config.slotCount = 4;
    config.inputSize = 8;
    config.maxPrediction = 12;
    config.checksumInterval = 5;
    config.seed = 11;
    config.assetHash = 22;
    config.pipelineHash = 33;
    config.buildId = 44;

    return config;
}

const std::array<std::byte, 6> kSixBytes{
    std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, std::byte{6}};

}

TEST_CASE("a hello survives the wire with the whole config")
{
    const unison::net::Hello sent{7, sampleConfig(), unison::net::Role::Spectator, 99};

    const auto received = decodedAs<unison::net::Hello>(encoded(sent));

    REQUIRE(received.protocolVersion == 7U);
    REQUIRE(unison::net::hashOf(received.config) == unison::net::hashOf(sampleConfig()));
    REQUIRE(received.role == unison::net::Role::Spectator);
    REQUIRE(received.reconnectToken == 99U);
}

TEST_CASE("a welcome survives the wire with the whole config")
{
    const unison::net::Welcome sent{2, sampleConfig(), 100, 140, 77};

    const auto received = decodedAs<unison::net::Welcome>(encoded(sent));

    REQUIRE(received.slot == 2U);
    REQUIRE(unison::net::hashOf(received.config) == unison::net::hashOf(sampleConfig()));
    REQUIRE(received.startFrame == 100U);
    REQUIRE(received.confirmedFrame == 140U);
    REQUIRE(received.reconnectToken == 77U);
}

TEST_CASE("an input batch survives the wire")
{
    const unison::net::Input sent{40, 2, 3, kSixBytes};

    const std::vector<std::byte> bytes = encoded(sent);
    const auto received = decodedAs<unison::net::Input>(bytes);

    REQUIRE(received.firstFrame == 40U);
    REQUIRE(received.inputSize == 2U);
    REQUIRE(received.frameCount == 3U);
    REQUIRE(std::ranges::equal(received.inputs, kSixBytes));
}

TEST_CASE("a batch of confirmed frames survives the wire")
{
    const unison::net::Confirmed sent{41, 1, 2, 2, kSixBytes};

    const std::vector<std::byte> bytes = encoded(sent);
    const auto received = decodedAs<unison::net::Confirmed>(bytes);

    REQUIRE(received.firstFrame == 41U);
    REQUIRE(received.slotCount == 1U);
    REQUIRE(received.inputSize == 2U);
    REQUIRE(received.frameCount == 2U);
    REQUIRE(std::ranges::equal(received.slots, kSixBytes));
}

TEST_CASE("as many confirmed frames as a datagram holds fit in one, and one more does not")
{
    constexpr std::uint8_t kSlots = 8;
    constexpr std::uint8_t kInputSize = 64;
    constexpr std::size_t kFrameSize = kSlots * (1U + kInputSize);
    const std::uint32_t fitting = unison::net::confirmedFramesPerDatagram(kSlots, kInputSize);
    const std::vector<std::byte> slots((fitting + 1U) * kFrameSize);
    std::vector<std::byte> datagram(unison::net::kMaxDatagramSize);

    const auto fits = unison::net::encode(
        unison::net::Confirmed{
            1, kSlots, kInputSize, static_cast<std::uint8_t>(fitting), std::span{slots}.first(fitting * kFrameSize)},
        datagram);
    const auto overflows = unison::net::encode(
        unison::net::Confirmed{1, kSlots, kInputSize, static_cast<std::uint8_t>(fitting + 1U), slots}, datagram);

    REQUIRE(fitting >= 1U);
    REQUIRE(fits.has_value());
    REQUIRE_FALSE(overflows.has_value());
}

TEST_CASE("frames without slots take no room, so a confirmation holds as many of them as its count can say")
{
    REQUIRE(unison::net::confirmedFramesPerDatagram(0, 8) == 255U);
}

TEST_CASE("a checksum survives the wire")
{
    const auto received = decodedAs<unison::net::Checksum>(encoded(unison::net::Checksum{60, 0x1122334455667788U}));

    REQUIRE(received.frame == 60U);
    REQUIRE(received.checksum == 0x1122334455667788U);
}

TEST_CASE("a desync survives the wire")
{
    const auto received = decodedAs<unison::net::Desync>(encoded(unison::net::Desync{61, 0b0000'0101U}));

    REQUIRE(received.frame == 61U);
    REQUIRE(received.minoritySlots == 0b0000'0101U);
}

TEST_CASE("a snapshot request survives the wire")
{
    const auto received = decodedAs<unison::net::SnapshotRequest>(encoded(unison::net::SnapshotRequest{62}));

    REQUIRE(received.frame == 62U);
}

TEST_CASE("a snapshot chunk survives the wire")
{
    const unison::net::SnapshotChunk sent{63, 1, 3, kSixBytes};

    const std::vector<std::byte> bytes = encoded(sent);
    const auto received = decodedAs<unison::net::SnapshotChunk>(bytes);

    REQUIRE(received.frame == 63U);
    REQUIRE(received.chunkIndex == 1U);
    REQUIRE(received.chunkCount == 3U);
    REQUIRE(std::ranges::equal(received.bytes, kSixBytes));
}

TEST_CASE("a ping survives the wire")
{
    const auto received = decodedAs<unison::net::Ping>(encoded(unison::net::Ping{123456}));

    REQUIRE(received.sentAt == 123456U);
}

TEST_CASE("a pong survives the wire")
{
    const auto received = decodedAs<unison::net::Pong>(encoded(unison::net::Pong{123456, 64, 70}));

    REQUIRE(received.pingSentAt == 123456U);
    REQUIRE(received.confirmedFrame == 64U);
    REQUIRE(received.dueFrame == 70U);
}

TEST_CASE("a leave survives the wire")
{
    const auto received = decodedAs<unison::net::Leave>(encoded(unison::net::Leave{unison::net::LeaveReason::Quit}));

    REQUIRE(received.reason == unison::net::LeaveReason::Quit);
}

TEST_CASE("a kick survives the wire")
{
    const auto received =
        decodedAs<unison::net::Kick>(encoded(unison::net::Kick{unison::net::LeaveReason::ConfigMismatch}));

    REQUIRE(received.reason == unison::net::LeaveReason::ConfigMismatch);
}

TEST_CASE("a message that does not fit the buffer is not written")
{
    std::array<std::byte, 4> tooSmall{};

    const auto written = unison::net::encode(unison::net::Checksum{60, 1}, tooSmall);

    REQUIRE_FALSE(written.has_value());
    REQUIRE(written.error().code() == unison::ErrorCode::BufferTooSmall);
}

TEST_CASE("bytes that end before their message does are rejected as truncated")
{
    std::vector<std::byte> bytes = encoded(unison::net::Checksum{60, 1});
    bytes.pop_back();

    REQUIRE(failureOf(bytes) == unison::ErrorCode::TruncatedInput);
}

TEST_CASE("no bytes at all are rejected as truncated")
{
    REQUIRE(failureOf({}) == unison::ErrorCode::TruncatedInput);
}

TEST_CASE("a message of a type the protocol does not have is rejected")
{
    const std::array<std::byte, 1> unknown{std::byte{0xEE}};

    REQUIRE(failureOf(unknown) == unison::ErrorCode::MalformedMessage);
}

TEST_CASE("a value outside its enumeration is rejected")
{
    std::vector<std::byte> bytes = encoded(unison::net::Leave{unison::net::LeaveReason::Quit});
    bytes.back() = std::byte{0x7F};

    REQUIRE(failureOf(bytes) == unison::ErrorCode::MalformedMessage);
}

TEST_CASE("bytes left over after a message are rejected")
{
    std::vector<std::byte> bytes = encoded(unison::net::SnapshotRequest{62});
    bytes.push_back(std::byte{0});

    REQUIRE(failureOf(bytes) == unison::ErrorCode::MalformedMessage);
}

TEST_CASE("an input batch promising more inputs than it carries is rejected as truncated")
{
    std::vector<std::byte> bytes = encoded(unison::net::Input{40, 2, 3, kSixBytes});
    bytes.resize(bytes.size() - 2);

    REQUIRE(failureOf(bytes) == unison::ErrorCode::TruncatedInput);
}

TEST_CASE("a batch of confirmed frames promising more frames than it carries is rejected as truncated")
{
    std::vector<std::byte> bytes = encoded(unison::net::Confirmed{41, 1, 2, 2, kSixBytes});
    bytes.resize(bytes.size() - 3);

    REQUIRE(failureOf(bytes) == unison::ErrorCode::TruncatedInput);
}

TEST_CASE("a snapshot chunk numbered past its count is rejected")
{
    constexpr std::size_t kChunkIndexOffset = sizeof(std::uint8_t) + sizeof(std::uint32_t);
    std::vector<std::byte> bytes = encoded(unison::net::SnapshotChunk{63, 1, 3, kSixBytes});
    bytes[kChunkIndexOffset] = std::byte{3};

    REQUIRE(failureOf(bytes) == unison::ErrorCode::MalformedMessage);
}

TEST_CASE("an input batch whose bytes disagree with its sizes breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    std::vector<std::byte> buffer(kRoomForAnyMessage);

    static_cast<void>(unison::net::encode(unison::net::Input{40, 2, 2, kSixBytes}, buffer));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a batch of confirmed frames whose bytes disagree with its sizes breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    std::vector<std::byte> buffer(kRoomForAnyMessage);

    static_cast<void>(unison::net::encode(unison::net::Confirmed{41, 3, 2, 1, kSixBytes}, buffer));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("as many snapshot bytes as a chunk holds fit in a datagram, and one more does not")
{
    const std::size_t fitting = unison::net::snapshotBytesPerChunk();
    const std::vector<std::byte> bytes(fitting + 1U);
    std::vector<std::byte> datagram(unison::net::kMaxDatagramSize);

    const auto fits =
        unison::net::encode(unison::net::SnapshotChunk{1, 0, 1, std::span{bytes}.first(fitting)}, datagram);
    const auto overflows = unison::net::encode(unison::net::SnapshotChunk{1, 0, 1, bytes}, datagram);

    REQUIRE(fitting >= 1U);
    REQUIRE(fits.has_value());
    REQUIRE_FALSE(overflows.has_value());
}
