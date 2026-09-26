#include <unison/replay/replay_report.hpp>

#include <unison/core/error.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/replay_verification.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

namespace
{

constexpr std::uint64_t kAssetHash = 0x1111;
constexpr std::uint64_t kPipelineHash = 0x2222;

unison::session::ReplayVerdict verdictOf(std::uint32_t compared, std::uint32_t matched)
{
    unison::session::ReplayVerdict verdict;
    verdict.framesPlayed = 360;
    verdict.checksumsCompared = compared;
    verdict.checksumsMatched = matched;

    return verdict;
}

unison::net::SessionConfig recordedWith(std::uint64_t assetHash, std::uint64_t pipelineHash)
{
    unison::net::SessionConfig config;
    config.assetHash = assetHash;
    config.pipelineHash = pipelineHash;

    return config;
}

}

TEST_CASE("verifying a replay whose checksums all match ends with 0")
{
    REQUIRE(unison::replay::verifyExitCodeOf(verdictOf(18, 18)) == 0);
}

TEST_CASE("verifying a replay with a checksum that differs ends with 2")
{
    REQUIRE(unison::replay::verifyExitCodeOf(verdictOf(18, 17)) == 2);
}

TEST_CASE("the verify report says how many checksums match over how many frames")
{
    const std::string report = unison::replay::verifyReportOf(verdictOf(18, 17));

    REQUIRE(report == "unison_replay: 17 of 18 checksums match over 360 frames\n");
}

TEST_CASE("the play report says how many frames were played and the last one's checksum")
{
    const std::string report = unison::replay::playReportOf(verdictOf(18, 18), 0x0123456789ABCDEFULL);

    REQUIRE(report == "unison_replay: played 360 frames; the last one's checksum is 0x0123456789abcdef\n");
}

TEST_CASE("a replay recorded with the build's assets and systems is accepted")
{
    REQUIRE(unison::replay::checkRecordedContent(recordedWith(kAssetHash, kPipelineHash), kAssetHash, kPipelineHash)
                .has_value());
}

TEST_CASE("a replay recorded with other assets or systems is refused")
{
    const auto otherAssets =
        unison::replay::checkRecordedContent(recordedWith(kAssetHash + 1, kPipelineHash), kAssetHash, kPipelineHash);
    const auto otherSystems =
        unison::replay::checkRecordedContent(recordedWith(kAssetHash, kPipelineHash + 1), kAssetHash, kPipelineHash);

    REQUIRE_FALSE(otherAssets.has_value());
    REQUIRE(otherAssets.error().code() == unison::ErrorCode::ForeignReplay);
    REQUIRE_FALSE(otherSystems.has_value());
    REQUIRE(otherSystems.error().code() == unison::ErrorCode::ForeignReplay);
}
