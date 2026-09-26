#include <unison/session/newest_inputs.hpp>

#include <unison/net/protocol.hpp>
#include <unison/session/session.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/session_script.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace
{

using unison::test::SampleInput;

constexpr std::uint8_t kLocalSlot = unison::test::kSessionLocalSlot;
constexpr std::uint8_t kInputSize = sizeof(SampleInput);

unison::net::SessionConfig threePlayers()
{
    unison::net::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = kInputSize;
    config.maxPrediction = 8;

    return config;
}

void playWithYaws(unison::session::Session& session, std::int16_t firstYaw, std::int16_t frames)
{
    for (std::int16_t yaw = firstYaw; yaw < firstYaw + frames; ++yaw)
    {
        const SampleInput local{0, 0, yaw, 0};
        session.setLocalInput(std::as_bytes(std::span{&local, 1}));
        session.tick();
    }
}

std::int16_t yawAt(const unison::net::Input& input, std::uint32_t offset)
{
    SampleInput played;
    std::memcpy(&played, input.inputs.subspan(offset * kInputSize, kInputSize).data(), kInputSize);

    return played.yaw;
}

}

TEST_CASE("a session's newest inputs are those of its newest frame and of the three before it")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    playWithYaws(session, 1, 6);
    std::array<std::byte, unison::session::kRedundantInputs * kInputSize> batch{};

    const std::optional<unison::net::Input> newest =
        unison::session::newestInputsOf(session, kLocalSlot, 0, kInputSize, batch);

    REQUIRE(newest.has_value());
    REQUIRE(newest->firstFrame == 3U);
    REQUIRE(newest->frameCount == 4U);
    REQUIRE(newest->inputSize == kInputSize);
    REQUIRE(yawAt(*newest, 0) == 3);
    REQUIRE(yawAt(*newest, 3) == 6);
}

TEST_CASE("a session's newest inputs begin after its verified frame")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    playWithYaws(session, 1, 6);
    for (std::uint32_t confirmed = 1; confirmed <= 4; ++confirmed)
    {
        static_cast<void>(session.confirm(confirmed, session.inputs().inputsAt(confirmed)));
    }
    std::array<std::byte, unison::session::kRedundantInputs * kInputSize> batch{};

    const std::optional<unison::net::Input> newest =
        unison::session::newestInputsOf(session, kLocalSlot, 0, kInputSize, batch);

    REQUIRE(newest.has_value());
    REQUIRE(newest->firstFrame == 5U);
    REQUIRE(newest->frameCount == 2U);
}

TEST_CASE("a session whose every frame is verified has no newest inputs to send")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    const unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot};
    std::array<std::byte, unison::session::kRedundantInputs * kInputSize> batch{};

    REQUIRE_FALSE(unison::session::newestInputsOf(session, kLocalSlot, 0, kInputSize, batch).has_value());
}

TEST_CASE("a session's newest inputs reach as far past its predicted frame as its input delay")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, threePlayers(), kLocalSlot, 2};
    playWithYaws(session, 1, 3);
    std::array<std::byte, unison::session::kRedundantInputs * kInputSize> batch{};

    const std::optional<unison::net::Input> newest =
        unison::session::newestInputsOf(session, kLocalSlot, 2, kInputSize, batch);

    REQUIRE(newest.has_value());
    REQUIRE(newest->firstFrame + newest->frameCount - 1U == 5U);
}
