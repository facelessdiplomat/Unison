#include <unison/replay/replay_report.hpp>

#include <unison/core/error.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/replay_verification.hpp>
#include <unison/session/state_diff.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <optional>
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

TEST_CASE("the verify report names the first frame whose checksum differs")
{
    unison::session::ReplayVerdict verdict = verdictOf(18, 17);
    verdict.firstDivergentFrame = 300;

    const std::string report = unison::replay::verifyReportOf(verdict);

    REQUIRE(report == "unison_replay: 17 of 18 checksums match over 360 frames; the first to differ is frame 300\n");
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

TEST_CASE("snapshots that are alike end the diff with 0 and say so")
{
    const std::optional<unison::session::StateDifference> alike;

    REQUIRE(unison::replay::diffExitCodeOf(alike) == 0);
    REQUIRE(unison::replay::diffReportOf(alike) == "unison_replay: the snapshots are alike\n");
}

TEST_CASE("snapshots that differ end the diff with 2")
{
    const std::optional<unison::session::StateDifference> difference{
        unison::session::StateDifference{"Health", 3, 0, "points"}};

    REQUIRE(unison::replay::diffExitCodeOf(difference) == 2);
}

TEST_CASE("the diff report names the component, the entity, the byte and the field")
{
    const std::optional<unison::session::StateDifference> difference{
        unison::session::StateDifference{"Health", 3, 0, "points"}};

    REQUIRE(unison::replay::diffReportOf(difference) ==
            "unison_replay: the snapshots first differ in Health of entity 3 at byte 0, in its field points\n");
}

TEST_CASE("the diff report names a pool whose entities part ways by the first one's entity")
{
    const std::optional<unison::session::StateDifference> difference{
        unison::session::StateDifference{"Health", 3, std::nullopt, std::nullopt}};

    REQUIRE(
        unison::replay::diffReportOf(difference) ==
        "unison_replay: the snapshots first differ in Health, where the first holds entity 3 and the second another\n");
}

TEST_CASE("the diff report names a part outside the pools and its byte")
{
    const std::optional<unison::session::StateDifference> difference{
        unison::session::StateDifference{"globals", std::nullopt, 5, std::nullopt}};

    REQUIRE(unison::replay::diffReportOf(difference) ==
            "unison_replay: the snapshots first differ in globals at byte 5\n");
}
