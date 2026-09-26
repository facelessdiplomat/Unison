#include <unison/session/replay_player.hpp>

#include <support/allocation_probe.hpp>
#include <support/session_script.hpp>
#include <unison/core/error.hpp>
#include <unison/session/replay_reader.hpp>
#include <unison/session/replay_writer.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace
{

using PlayedStep = tl::expected<std::optional<unison::session::VerifiedChecksum>, unison::Error>;

constexpr std::uint32_t kFrames = 40;
constexpr std::uint32_t kChecksumInterval = 4;
constexpr std::uint32_t kWarmUpFrames = 240;
constexpr std::uint32_t kMeasuredFrames = 240;

unison::net::SessionConfig scriptedConfig()
{
    unison::net::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = 8;
    config.checksumInterval = kChecksumInterval;

    return config;
}

class ScriptedMatch
{
public:
    ScriptedMatch()
    {
        unison::test::addScoredEntity(frame);
        pipeline.add(mixer);
        pipeline.add(moves);
        pipeline.add(settles);
    }

    unison::sim::Frame frame;
    unison::test::InputMixer mixer;
    unison::test::MoveAnnouncer moves;
    unison::test::SettleAnnouncer settles;
    unison::sim::SystemPipeline pipeline;
};

std::vector<std::byte> scriptedReplay()
{
    ScriptedMatch match;
    unison::session::ReplayWriter writer{scriptedConfig()};

    for (std::uint32_t next = 1; next <= kFrames; ++next)
    {
        const unison::sim::FrameInputs inputs = unison::test::scriptedSessionInputs(next);
        writer.writeFrame(next, inputs);
        unison::sim::advanceFrame(match.frame, match.pipeline, inputs);

        if (next % kChecksumInterval == 0)
        {
            writer.writeChecksum(unison::session::VerifiedChecksum{next, unison::sim::checksumOf(match.frame)});
        }
    }

    return {writer.bytes().begin(), writer.bytes().end()};
}

std::vector<unison::session::VerifiedChecksum> recordedChecksums(std::span<const std::byte> replay)
{
    tl::expected<unison::session::ReplayReader, unison::Error> reader = unison::session::ReplayReader::open(replay);
    std::vector<unison::session::VerifiedChecksum> recorded;

    while (reader.has_value() && !reader->isAtEnd())
    {
        const tl::expected<unison::session::ReplayRecord, unison::Error> record = reader->next();

        if (!record.has_value())
        {
            break;
        }

        if (const auto* checksum = std::get_if<unison::session::VerifiedChecksum>(&*record))
        {
            recorded.push_back(*checksum);
        }
    }

    return recorded;
}

std::vector<unison::session::VerifiedChecksum> playedChecksums(std::span<const std::byte> replay)
{
    tl::expected<unison::session::ReplayReader, unison::Error> reader = unison::session::ReplayReader::open(replay);

    if (!reader.has_value())
    {
        return {};
    }

    ScriptedMatch match;
    unison::session::ReplayPlayer player{match.frame, match.pipeline, reader->config()};
    std::vector<unison::session::VerifiedChecksum> played;

    while (!reader->isAtEnd())
    {
        const tl::expected<unison::session::ReplayRecord, unison::Error> record = reader->next();

        if (!record.has_value())
        {
            break;
        }

        const auto* frame = std::get_if<unison::session::ReplayFrame>(&*record);

        if (frame == nullptr)
        {
            continue;
        }

        const PlayedStep step = player.play(*frame);

        if (!step.has_value())
        {
            break;
        }

        if (step->has_value())
        {
            played.push_back(**step);
        }
    }

    return played;
}

}

TEST_CASE("a replay played back takes the checksums the match recorded")
{
    const std::vector<std::byte> replay = scriptedReplay();

    const std::vector<unison::session::VerifiedChecksum> played = playedChecksums(replay);

    REQUIRE(played.size() == kFrames / kChecksumInterval);
    REQUIRE(played == recordedChecksums(replay));
}

TEST_CASE("a replay frame that does not follow the last one played is refused")
{
    ScriptedMatch match;
    unison::session::ReplayPlayer player{match.frame, match.pipeline, scriptedConfig()};
    REQUIRE(player.play(unison::session::ReplayFrame{1, unison::test::scriptedSessionInputs(1)}).has_value());

    const PlayedStep skipped = player.play(unison::session::ReplayFrame{3, unison::test::scriptedSessionInputs(3)});

    REQUIRE_FALSE(skipped.has_value());
    REQUIRE(skipped.error().code() == unison::ErrorCode::MalformedReplay);
}

TEST_CASE("a replay player takes nothing from the C++ heap once warmed up")
{
    ScriptedMatch match;
    unison::session::ReplayPlayer player{match.frame, match.pipeline, scriptedConfig()};

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
