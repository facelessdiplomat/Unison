#include <unison/session/session.hpp>

#include <support/test_components.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace
{

struct SampleInput
{
    std::int8_t moveX = 0;
    std::int8_t moveY = 0;
    std::int16_t yaw = 0;
    std::uint32_t buttons = 0;
};

constexpr std::size_t kSlots = 3;
constexpr std::size_t kLocalSlot = 1;
constexpr std::uint32_t kFrames = 60;
constexpr std::uint32_t kConfirmationDelay = 3;

class InputMixer final : public unison::sim::ISystem
{
public:
    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override
    {
        for (auto [entity, health] : frame.registry.view<unison::test::Health>().each())
        {
            auto mixed = static_cast<std::uint32_t>(health.points);

            for (std::size_t slot = 0; slot < kSlots; ++slot)
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

class InputRecorder final : public unison::sim::ISystem
{
public:
    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override
    {
        played.at(frame.frameNumber + 1) = inputs;
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "InputRecorder";
    }

    [[nodiscard]] const unison::sim::FrameInputs& playedAt(std::uint32_t frameNumber) const
    {
        return played.at(frameNumber);
    }

private:
    std::array<unison::sim::FrameInputs, kFrames + 2> played{};
};

unison::session::SessionConfig threePlayers()
{
    unison::session::SessionConfig config;
    config.slotCount = kSlots;
    config.inputSize = sizeof(SampleInput);
    config.maxPrediction = 8;

    return config;
}

SampleInput inputWithMove(std::int8_t moveX)
{
    SampleInput input;
    input.moveX = moveX;

    return input;
}

std::span<const std::byte> bytesOf(const SampleInput& input)
{
    return std::as_bytes(std::span{&input, 1});
}

SampleInput trueInput(std::uint32_t frameNumber, std::size_t slot)
{
    if (slot == kLocalSlot)
    {
        return inputWithMove(static_cast<std::int8_t>(frameNumber % 5U));
    }

    return inputWithMove(static_cast<std::int8_t>(static_cast<int>((frameNumber / 4U + slot) % 3U) - 1));
}

unison::sim::FrameInputs trueInputs(std::uint32_t frameNumber)
{
    unison::sim::FrameInputs inputs;

    for (std::size_t slot = 0; slot < kSlots; ++slot)
    {
        inputs.set(slot, trueInput(frameNumber, slot), unison::sim::InputFlags::Present);
    }

    return inputs;
}

void addScoredEntity(unison::sim::Frame& frame)
{
    frame.registry.emplace<unison::test::Health>(frame.registry.create(), 1);
}

std::uint64_t checksumOfStraightLine(std::uint32_t frames)
{
    unison::sim::Frame frame;
    addScoredEntity(frame);
    InputMixer mixer;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(mixer);

    for (std::uint32_t next = 1; next <= frames; ++next)
    {
        unison::sim::advanceFrame(frame, pipeline, trueInputs(next));
    }

    return unison::sim::checksumOf(frame);
}

unison::sim::FrameInputs firstFrameWithSlotZeroMoving()
{
    unison::sim::FrameInputs confirmed;
    confirmed.set(0, inputWithMove(7), unison::sim::InputFlags::Present);
    confirmed.set(kLocalSlot, inputWithMove(1), unison::sim::InputFlags::Present);
    confirmed.set(2, SampleInput{}, unison::sim::InputFlags::None);

    return confirmed;
}

}

TEST_CASE("a session that guessed wrong ends where a match played on the true inputs does")
{
    unison::sim::Frame frame;
    addScoredEntity(frame);
    InputMixer mixer;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(mixer);
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};

    for (std::uint32_t next = 1; next <= kFrames; ++next)
    {
        const SampleInput local = trueInput(next, kLocalSlot);
        session.setLocalInput(bytesOf(local));
        session.tick();

        if (next > kConfirmationDelay)
        {
            REQUIRE(session.confirm(next - kConfirmationDelay, trueInputs(next - kConfirmationDelay)));
        }
    }

    for (std::uint32_t late = kFrames - kConfirmationDelay + 1; late <= kFrames + 1; ++late)
    {
        REQUIRE(session.confirm(late, trueInputs(late)));
    }

    session.tick();

    REQUIRE(mixer.runCount() > kFrames + 1);
    REQUIRE(session.verifiedFrame() == kFrames + 1);
    REQUIRE(unison::sim::checksumOf(frame) == checksumOfStraightLine(kFrames + 1));
}

TEST_CASE("the verified frame waits for a frame guessed wrong to be played again")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    const SampleInput local = inputWithMove(1);
    session.setLocalInput(bytesOf(local));
    session.tick();
    session.tick();

    const bool confirmed = session.confirm(1, firstFrameWithSlotZeroMoving());

    REQUIRE(confirmed);
    REQUIRE(session.verifiedFrame() == 0U);
}

TEST_CASE("a frame guessed wrong is verified once it has been played again")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    const SampleInput local = inputWithMove(1);
    session.setLocalInput(bytesOf(local));
    session.tick();
    session.tick();
    REQUIRE(session.confirm(1, firstFrameWithSlotZeroMoving()));

    session.tick();

    REQUIRE(session.verifiedFrame() == 1U);
}

TEST_CASE("a rollback plays the local player's inputs again as they were first played")
{
    unison::sim::Frame frame;
    InputRecorder recorder;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(recorder);
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    const SampleInput first = inputWithMove(1);
    const SampleInput second = inputWithMove(2);
    const SampleInput third = inputWithMove(3);
    session.setLocalInput(bytesOf(first));
    session.tick();
    session.setLocalInput(bytesOf(second));
    session.tick();
    session.setLocalInput(bytesOf(third));
    REQUIRE(session.confirm(1, firstFrameWithSlotZeroMoving()));

    session.tick();

    REQUIRE(recorder.playedAt(2).get<SampleInput>(kLocalSlot).moveX == 2);
    REQUIRE(recorder.playedAt(3).get<SampleInput>(kLocalSlot).moveX == 3);
}

TEST_CASE("a rollback guesses the frames it plays again from the input just confirmed")
{
    unison::sim::Frame frame;
    InputRecorder recorder;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(recorder);
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    const SampleInput local = inputWithMove(1);
    session.setLocalInput(bytesOf(local));
    session.tick();
    session.tick();
    REQUIRE(session.confirm(1, firstFrameWithSlotZeroMoving()));

    session.tick();

    REQUIRE(recorder.playedAt(2).get<SampleInput>(0).moveX == 7);
    REQUIRE(recorder.playedAt(2).flagsAt(0) == unison::sim::InputFlags::Present);
}
