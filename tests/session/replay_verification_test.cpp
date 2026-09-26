#include <unison/session/replay_verification.hpp>

#include <support/replay_script.hpp>
#include <support/session_script.hpp>
#include <unison/core/error.hpp>
#include <unison/session/replay_player.hpp>
#include <unison/session/replay_reader.hpp>
#include <unison/session/replay_writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace
{

constexpr std::uint32_t kFrames = 40;
constexpr std::uint32_t kMiscountedFrame = 8;
constexpr std::uint32_t kLongReplayFrames = 320;
constexpr std::uint32_t kTamperedFrame = 302;
constexpr std::uint32_t kFirstChecksumAfterTheTampering = 304;

tl::expected<unison::session::ReplayVerdict, unison::Error> verified(std::span<const std::byte> replay)
{
    tl::expected<unison::session::ReplayReader, unison::Error> reader = unison::session::ReplayReader::open(replay);

    if (!reader.has_value())
    {
        return tl::unexpected{reader.error()};
    }

    unison::test::ScriptedMatch match;
    unison::session::ReplayPlayer player{match.frame, match.pipeline, reader->config()};

    return unison::session::verifyReplay(*reader, player);
}

}

TEST_CASE("a replay played back matches every checksum it recorded")
{
    const std::vector<std::byte> replay = unison::test::scriptedReplay(kFrames);

    const tl::expected<unison::session::ReplayVerdict, unison::Error> verdict = verified(replay);

    REQUIRE(verdict.has_value());
    REQUIRE(verdict->framesPlayed == kFrames);
    REQUIRE(verdict->checksumsCompared == kFrames / unison::test::kScriptedReplayChecksumInterval);
    REQUIRE(verdict->checksumsMatched == verdict->checksumsCompared);
    REQUIRE_FALSE(verdict->firstDivergentFrame.has_value());
}

TEST_CASE("a checksum recorded wrong is compared and does not match")
{
    const std::vector<std::byte> replay =
        unison::test::scriptedReplay(kFrames, unison::test::ReplayTampering{.miscountedChecksumAt = kMiscountedFrame});

    const tl::expected<unison::session::ReplayVerdict, unison::Error> verdict = verified(replay);

    REQUIRE(verdict.has_value());
    REQUIRE(verdict->checksumsCompared == kFrames / unison::test::kScriptedReplayChecksumInterval);
    REQUIRE(verdict->checksumsMatched == verdict->checksumsCompared - 1U);
}

TEST_CASE("a tampered input is reported at the first checksum taken at or after its frame")
{
    const std::vector<std::byte> replay = unison::test::scriptedReplay(
        kLongReplayFrames, unison::test::ReplayTampering{.tamperedInputAt = kTamperedFrame});

    const tl::expected<unison::session::ReplayVerdict, unison::Error> verdict = verified(replay);

    REQUIRE(verdict.has_value());
    REQUIRE(verdict->firstDivergentFrame == kFirstChecksumAfterTheTampering);
}

TEST_CASE("a checksum of a frame the replay did not just play is refused")
{
    unison::session::ReplayWriter writer{unison::test::scriptedReplayConfig()};

    for (std::uint32_t next = 1; next <= unison::test::kScriptedReplayChecksumInterval; ++next)
    {
        writer.writeFrame(next, unison::test::scriptedSessionInputs(next));
    }

    writer.writeChecksum(unison::session::VerifiedChecksum{2, 0});

    const tl::expected<unison::session::ReplayVerdict, unison::Error> verdict = verified(writer.bytes());

    REQUIRE_FALSE(verdict.has_value());
    REQUIRE(verdict.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a record the reader refuses stops the verification with the reader's error")
{
    const std::vector<std::byte> replay = unison::test::scriptedReplay(kFrames);

    const tl::expected<unison::session::ReplayVerdict, unison::Error> verdict =
        verified(std::span{replay}.first(replay.size() - 1));

    REQUIRE_FALSE(verdict.has_value());
    REQUIRE(verdict.error().code() == unison::ErrorCode::TruncatedInput);
}
