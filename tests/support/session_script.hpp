#pragma once

#include <support/test_components.hpp>
#include <unison/session/session.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/event_buffer.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace unison::test
{

/// A game input small enough for every session test: plain data, quantised, free of padding.
struct SampleInput
{
    std::int8_t moveX = 0;
    std::int8_t moveY = 0;
    std::int16_t yaw = 0;
    std::uint32_t buttons = 0;
};

/// How many players the scripted session seats, and which of them is the local one.
inline constexpr std::size_t kSessionSlots = 3;
inline constexpr std::size_t kSessionLocalSlot = 1;

[[nodiscard]] inline std::span<const std::byte> bytesOf(const SampleInput& input)
{
    return std::as_bytes(std::span{&input, 1});
}

[[nodiscard]] inline SampleInput inputWithMove(std::int8_t moveX)
{
    SampleInput input;
    input.moveX = moveX;

    return input;
}

/// Folds every slot's input and flags into the health of each scored entity, so a frame played on any
/// other input ends with a different checksum.
class InputMixer final : public sim::ISystem
{
public:
    void update(sim::Frame& frame, const sim::FrameInputs& inputs) override
    {
        for (auto [entity, health] : frame.registry.view<Health>().each())
        {
            auto mixed = static_cast<std::uint32_t>(health.points);

            for (std::size_t slot = 0; slot < kSessionSlots; ++slot)
            {
                mixed = mixed * 31U + static_cast<std::uint32_t>(inputs.get<SampleInput>(slot).moveX) +
                        static_cast<std::uint32_t>(inputs.flagsAt(slot));
            }

            health.points = static_cast<std::int32_t>(mixed);
        }

        ++runs;
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "InputMixer";
    }

    [[nodiscard]] std::uint32_t runCount() const
    {
        return runs;
    }

private:
    std::uint32_t runs = 0;
};

/// Raised for every slot whose input moves, and shown to the view at once.
struct SlotMoved
{
    std::uint32_t slot = 0;
};

/// An event the view may only see once the frame that raised it is verified.
struct SlotSettled
{
    std::uint32_t slot = 0;
};

}

UNISON_EVENT(unison::test::SlotMoved, unison::sim::EventKind::Predicted);
UNISON_EVENT(unison::test::SlotSettled, unison::sim::EventKind::VerifiedOnly);

namespace unison::test
{

/// Raises SlotMoved for every slot whose input moves on the frame being played.
class MoveAnnouncer final : public sim::ISystem
{
public:
    void update(sim::Frame& frame, const sim::FrameInputs& inputs) override
    {
        for (std::uint32_t slot = 0; slot < kSessionSlots; ++slot)
        {
            if (inputs.get<SampleInput>(slot).moveX != 0)
            {
                frame.events.raise(frame.frameNumber, SlotMoved{slot});
            }
        }
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "MoveAnnouncer";
    }
};

/// Remembers the inputs each frame was last played on and how often the pipeline ran.
class InputRecorder final : public sim::ISystem
{
public:
    void update(sim::Frame& frame, const sim::FrameInputs& inputs) override
    {
        const std::size_t made = frame.frameNumber + 1U;

        if (played.size() <= made)
        {
            played.resize(made + 1U);
        }

        played[made] = inputs;
        latest = inputs;
        ++runs;
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "InputRecorder";
    }

    [[nodiscard]] const sim::FrameInputs& playedAt(std::uint32_t frameNumber) const
    {
        return played.at(frameNumber);
    }

    [[nodiscard]] const sim::FrameInputs& lastPlayed() const
    {
        return latest;
    }

    [[nodiscard]] std::uint32_t runCount() const
    {
        return runs;
    }

private:
    std::vector<sim::FrameInputs> played;
    sim::FrameInputs latest;
    std::uint32_t runs = 0;
};

/// What each player of the scripted session really asks for on a frame: the local player changes every
/// frame, the others every fourth, so a guess that repeats the last input is often wrong.
[[nodiscard]] inline SampleInput scriptedSessionInput(std::uint32_t frameNumber, std::size_t slot)
{
    if (slot == kSessionLocalSlot)
    {
        return inputWithMove(static_cast<std::int8_t>(frameNumber % 5U));
    }

    return inputWithMove(static_cast<std::int8_t>(static_cast<int>((frameNumber / 4U + slot) % 3U) - 1));
}

[[nodiscard]] inline sim::FrameInputs scriptedSessionInputs(std::uint32_t frameNumber)
{
    sim::FrameInputs inputs;

    for (std::size_t slot = 0; slot < kSessionSlots; ++slot)
    {
        inputs.set(slot, scriptedSessionInput(frameNumber, slot), sim::InputFlags::Present);
    }

    return inputs;
}

inline void addScoredEntity(sim::Frame& frame)
{
    frame.registry.emplace<Health>(frame.registry.create(), 1);
}

/// Confirms a frame exactly as the session played it, which is how the relay settles a frame guessed right.
[[nodiscard]] inline bool confirmAsPlayed(session::Session& session, std::uint32_t frameNumber)
{
    const sim::FrameInputs asPlayed = session.inputs().inputsAt(frameNumber);

    return session.confirm(frameNumber, asPlayed);
}

/// The checksum the scripted session reaches at a frame when played straight through on the true inputs.
[[nodiscard]] inline std::uint64_t checksumOfScriptedSession(std::uint32_t frames)
{
    sim::Frame frame;
    addScoredEntity(frame);
    InputMixer mixer;
    sim::SystemPipeline pipeline;
    pipeline.add(mixer);

    for (std::uint32_t next = 1; next <= frames; ++next)
    {
        sim::advanceFrame(frame, pipeline, scriptedSessionInputs(next));
    }

    return sim::checksumOf(frame);
}

}
