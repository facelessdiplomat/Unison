#include <unison/net/message_codec.hpp>
#include <unison/net/protocol.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

constexpr std::array<std::byte, 4> kInputBytes{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
constexpr std::array<std::byte, 4> kConfirmedSlots{std::byte{0x01}, std::byte{0xAA}, std::byte{0x02}, std::byte{0xBB}};
constexpr std::array<std::byte, 2> kChunkBytes{std::byte{0xDE}, std::byte{0xAD}};

std::string goldenPath()
{
    return std::string{UNISON_GOLDEN_DIR} + "/protocol.bytes";
}

unison::net::SessionConfig fixedConfig()
{
    unison::net::SessionConfig config;
    config.tickRate = 0x0102;
    config.slotCount = 0x03;
    config.inputSize = 0x04;
    config.maxPrediction = 0x05060708;
    config.checksumInterval = 0x090A0B0C;
    config.seed = 0x1112131415161718;
    config.assetHash = 0x2122232425262728;
    config.pipelineHash = 0x3132333435363738;
    config.buildId = 0x4142434445464748;

    return config;
}

std::vector<std::pair<std::string_view, unison::net::Message>> fixedMessages()
{
    return {
        {"Hello", unison::net::Hello{0x5152, fixedConfig(), unison::net::Role::Spectator, 0x6162636465666768}},
        {"Welcome", unison::net::Welcome{0x07, fixedConfig(), 0x71727374, 0x75767778, 0x8182838485868788}},
        {"Input", unison::net::Input{0x91929394, 0x02, 0x02, kInputBytes}},
        {"Confirmed", unison::net::Confirmed{0xA1A2A3A4, 0x02, 0x01, 0x01, kConfirmedSlots}},
        {"Checksum", unison::net::Checksum{0xB1B2B3B4, 0xB5B6B7B8B9BABBBC}},
        {"Desync", unison::net::Desync{0xC1C2C3C4, 0x05}},
        {"SnapshotRequest", unison::net::SnapshotRequest{0xD1D2D3D4}},
        {"SnapshotChunk", unison::net::SnapshotChunk{0xE1E2E3E4, 0x00000001, 0x00000003, kChunkBytes}},
        {"Ping", unison::net::Ping{0xF1F2F3F4F5F6F7F8}},
        {"Pong", unison::net::Pong{0x0A0B0C0D0E0F1011, 0x12131415, 0x16171819}},
        {"Leave", unison::net::Leave{unison::net::LeaveReason::ConfigMismatch}},
        {"Kick", unison::net::Kick{unison::net::LeaveReason::RoomFull}},
    };
}

std::string hexOf(std::span<const std::byte> bytes)
{
    constexpr std::string_view kDigits = "0123456789abcdef";
    std::string hex;

    for (const std::byte value : bytes)
    {
        hex += kDigits[std::to_integer<std::size_t>(value >> 4U)];
        hex += kDigits[std::to_integer<std::size_t>(value & std::byte{0x0F})];
    }

    return hex;
}

std::string encodedHexOf(const unison::net::Message& message)
{
    std::array<std::byte, unison::net::kMaxDatagramSize> buffer{};
    const auto written = unison::net::encode(message, buffer);

    return written.has_value() ? hexOf(std::span{buffer}.first(*written)) : std::string{};
}

std::map<std::string, std::string> readGolden()
{
    std::ifstream file{goldenPath()};
    std::map<std::string, std::string> recorded;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line.front() == '#')
        {
            continue;
        }

        std::istringstream reader{line};
        std::string name;
        std::string hex;

        reader >> name >> hex;
        recorded[name] = hex;
    }

    return recorded;
}

}

TEST_CASE("every protocol message encodes to the bytes recorded for it")
{
    const std::map<std::string, std::string> golden = readGolden();
    const auto messages = fixedMessages();

    REQUIRE(golden.size() == messages.size());

    for (const auto& [name, message] : messages)
    {
        INFO(name);
        REQUIRE(golden.contains(std::string{name}));
        REQUIRE(encodedHexOf(message) == golden.at(std::string{name}));
    }
}

TEST_CASE("recording the protocol's bytes", "[.record]")
{
    std::ofstream file{goldenPath()};

    file << "# every message of the relay protocol encoded from fixed values: its name, then its bytes in hex\n";
    file << "# re-recorded on purpose only, with the protocol version: unison_tests_fast \"recording the protocol's "
            "bytes\"\n";

    for (const auto& [name, message] : fixedMessages())
    {
        file << name << ' ' << encodedHexOf(message) << '\n';
    }

    REQUIRE(file.good());
}
