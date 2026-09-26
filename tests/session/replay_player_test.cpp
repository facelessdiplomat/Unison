#include <unison/session/replay_player.hpp>

#include <support/allocation_probe.hpp>
#include <support/replay_playback.hpp>
#include <support/replay_script.hpp>
#include <support/session_script.hpp>
#include <unison/core/error.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace
{

constexpr std::uint32_t kFrames = 40;
constexpr std::uint32_t kWarmUpFrames = 240;
constexpr std::uint32_t kMeasuredFrames = 240;

}

TEST_CASE("a replay played back takes the checksums the match recorded")
{
    const std::vector<std::byte> replay = unison::test::scriptedReplay(kFrames);
    unison::test::ScriptedMatch match;

    const std::vector<unison::session::VerifiedChecksum> played =
        unison::test::playedChecksums(replay, match.frame, match.pipeline);

    REQUIRE(played.size() == kFrames / unison::test::kScriptedReplayChecksumInterval);
    REQUIRE(played == unison::test::recordedChecksums(replay));
}

TEST_CASE("a replay frame that does not follow the last one played is refused")
{
    unison::test::ScriptedMatch match;
    unison::session::ReplayPlayer player{match.frame, match.pipeline, unison::test::scriptedReplayConfig()};
    REQUIRE(player.play(unison::session::ReplayFrame{1, unison::test::scriptedSessionInputs(1)}).has_value());

    const unison::test::PlayedStep skipped =
        player.play(unison::session::ReplayFrame{3, unison::test::scriptedSessionInputs(3)});

    REQUIRE_FALSE(skipped.has_value());
    REQUIRE(skipped.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay player takes nothing from the C++ heap once warmed up")
{
    unison::test::ScriptedMatch match;
    unison::session::ReplayPlayer player{match.frame, match.pipeline, unison::test::scriptedReplayConfig()};

    for (std::uint32_t next = 1; next <= kWarmUpFrames; ++next)
    {
        REQUIRE(player.play(unison::session::ReplayFrame{next, unison::test::scriptedSessionInputs(next)}).has_value());
    }

    std::uint32_t played = 0;
    const unison::test::AllocationProbe probe;

    for (std::uint32_t next = kWarmUpFrames + 1; next <= kWarmUpFrames + kMeasuredFrames; ++next)
    {
        played += player.play(unison::session::ReplayFrame{next, unison::test::scriptedSessionInputs(next)}).has_value()
                      ? 1U
                      : 0U;
    }

    REQUIRE(probe.cppAllocations() == 0U);
    REQUIRE(played == kMeasuredFrames);
}
