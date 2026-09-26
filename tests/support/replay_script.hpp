#pragma once

#include <support/session_script.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/replay_writer.hpp>
#include <unison/session/verified_checksum.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace unison::test
{

/// How many verified frames lie between the checksums of a scripted replay.
inline constexpr std::uint32_t kScriptedReplayChecksumInterval = 4;

/// The config the scripted session is recorded with.
[[nodiscard]] inline net::SessionConfig scriptedReplayConfig()
{
    net::SessionConfig config;
    config.slotCount = kSessionSlots;
    config.inputSize = sizeof(SampleInput);
    config.maxPrediction = 8;
    config.checksumInterval = kScriptedReplayChecksumInterval;

    return config;
}

/// The scripted session's game as it begins: a scored entity, the input mixer and both event announcers.
class ScriptedMatch
{
public:
    ScriptedMatch()
    {
        addScoredEntity(frame);
        pipeline.add(mixer);
        pipeline.add(moves);
        pipeline.add(settles);
    }

    sim::Frame frame;
    InputMixer mixer;
    MoveAnnouncer moves;
    SettleAnnouncer settles;
    sim::SystemPipeline pipeline;
};

/// What a scripted replay gets wrong on purpose: the checksum it records for one frame, one off, and the input it
/// records for one frame, whose checksums still come from the true inputs. Nought leaves either alone.
struct ReplayTampering
{
    std::uint32_t miscountedChecksumAt = 0;
    std::uint32_t tamperedInputAt = 0;
};

/// A replay of the scripted session played straight through, each frame followed by its checksum on the interval,
/// with the tampering asked for.
[[nodiscard]] inline std::vector<std::byte> scriptedReplay(std::uint32_t frames, const ReplayTampering& tampering = {})
{
    ScriptedMatch match;
    session::ReplayWriter writer{scriptedReplayConfig()};

    for (std::uint32_t next = 1; next <= frames; ++next)
    {
        const sim::FrameInputs inputs = scriptedSessionInputs(next);
        sim::FrameInputs recorded = inputs;

        if (next == tampering.tamperedInputAt)
        {
            recorded.set(0,
                         inputWithMove(static_cast<std::int8_t>(inputs.get<SampleInput>(0).moveX + 1)),
                         sim::InputFlags::Present);
        }

        writer.writeFrame(next, recorded);
        sim::advanceFrame(match.frame, match.pipeline, inputs);

        if (next % kScriptedReplayChecksumInterval == 0)
        {
            const std::uint64_t checksum =
                sim::checksumOf(match.frame) + (next == tampering.miscountedChecksumAt ? 1U : 0U);
            writer.writeChecksum(session::VerifiedChecksum{next, checksum});
        }
    }

    return {writer.bytes().begin(), writer.bytes().end()};
}

}
