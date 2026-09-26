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

/// A replay of the scripted session played straight through, each frame followed by its checksum on the interval;
/// the checksum recorded for `miscountedFrame`, if it falls on the interval, is one off.
[[nodiscard]] inline std::vector<std::byte> scriptedReplay(std::uint32_t frames, std::uint32_t miscountedFrame = 0)
{
    ScriptedMatch match;
    session::ReplayWriter writer{scriptedReplayConfig()};

    for (std::uint32_t next = 1; next <= frames; ++next)
    {
        const sim::FrameInputs inputs = scriptedSessionInputs(next);
        writer.writeFrame(next, inputs);
        sim::advanceFrame(match.frame, match.pipeline, inputs);

        if (next % kScriptedReplayChecksumInterval == 0)
        {
            const std::uint64_t checksum = sim::checksumOf(match.frame) + (next == miscountedFrame ? 1U : 0U);
            writer.writeChecksum(session::VerifiedChecksum{next, checksum});
        }
    }

    return {writer.bytes().begin(), writer.bytes().end()};
}

}
