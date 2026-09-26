#include <unison/session/session.hpp>
#include <unison/session/verified_frame_receiver.hpp>

#include <support/session_script.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace
{

constexpr std::uint32_t kPlayedFrames = 12;
constexpr std::uint32_t kConfirmedAtOnce = 8;
constexpr std::uint32_t kChecksumInterval = 4;

struct HeardFrame
{
    std::uint32_t frameNumber = 0;
    unison::sim::FrameInputs inputs;
    std::optional<std::uint64_t> checksum;
};

class FrameListener final : public unison::session::IVerifiedFrameReceiver
{
public:
    void frameVerified(std::uint32_t frameNumber,
                       const unison::sim::FrameInputs& inputs,
                       std::optional<std::uint64_t> checksum) override
    {
        heardFrames.push_back(HeardFrame{frameNumber, inputs, checksum});
    }

    [[nodiscard]] const std::vector<HeardFrame>& heard() const
    {
        return heardFrames;
    }

private:
    std::vector<HeardFrame> heardFrames;
};

unison::net::SessionConfig scriptedConfig()
{
    unison::net::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = kPlayedFrames;
    config.checksumInterval = kChecksumInterval;

    return config;
}

bool isHeardAsPlayedStraightThrough(const HeardFrame& heard, std::uint32_t frameNumber)
{
    const unison::sim::FrameInputs expected = unison::test::scriptedSessionInputs(frameNumber);
    const std::optional<std::uint64_t> expectedChecksum =
        frameNumber % kChecksumInterval == 0 ? std::optional{unison::test::checksumOfScriptedSession(frameNumber)}
                                             : std::nullopt;

    for (std::size_t slot = 0; slot < unison::test::kSessionSlots; ++slot)
    {
        if (heard.inputs.flagsAt(slot) != expected.flagsAt(slot) ||
            !std::ranges::equal(heard.inputs.bytesAt(slot).first(sizeof(unison::test::SampleInput)),
                                expected.bytesAt(slot).first(sizeof(unison::test::SampleInput))))
        {
            return false;
        }
    }

    return heard.frameNumber == frameNumber && heard.checksum == expectedChecksum;
}

}

TEST_CASE("a session tells its receiver of every frame it verifies in order with its inputs and checksum")
{
    unison::sim::Frame frame;
    unison::test::addScoredEntity(frame);
    unison::test::InputMixer mixer;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(mixer);
    FrameListener listener;
    unison::session::Session session{frame, pipeline, scriptedConfig(), unison::test::kSessionLocalSlot, 0, &listener};

    for (std::uint32_t next = 1; next <= kPlayedFrames; ++next)
    {
        session.setLocalInput(
            unison::test::bytesOf(unison::test::scriptedSessionInput(next, unison::test::kSessionLocalSlot)));
        session.tick();
    }

    for (std::uint32_t confirmed = 1; confirmed <= kConfirmedAtOnce; ++confirmed)
    {
        REQUIRE(session.confirm(confirmed, unison::test::scriptedSessionInputs(confirmed)));
    }

    session.tick();

    REQUIRE(listener.heard().size() == kConfirmedAtOnce);

    for (std::uint32_t frameNumber = 1; frameNumber <= kConfirmedAtOnce; ++frameNumber)
    {
        CAPTURE(frameNumber);
        REQUIRE(isHeardAsPlayedStraightThrough(listener.heard()[frameNumber - 1], frameNumber));
    }
}
