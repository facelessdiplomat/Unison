#include <unison/session/session.hpp>

#include <unison/session/verified_checksum.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/session_script.hpp>

#include <cstdint>
#include <span>

namespace
{

unison::net::SessionConfig threePlayers()
{
    unison::net::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = 4;
    config.checksumInterval = 1;

    return config;
}

struct ScriptedGame
{
    ScriptedGame()
    {
        unison::test::addScoredEntity(frame);
        pipeline.add(mixer);
    }

    unison::sim::Frame frame;
    unison::test::InputMixer mixer;
    unison::sim::SystemPipeline pipeline;
};

struct Watched
{
    explicit Watched(std::uint32_t delayFrames = 0)
        : session{game.frame, game.pipeline, threePlayers(), unison::session::Spectating{delayFrames}}
    {
    }

    void confirmUpTo(std::uint32_t lastFrame)
    {
        for (std::uint32_t confirmed = 1; confirmed <= lastFrame; ++confirmed)
        {
            static_cast<void>(session.confirm(confirmed, unison::test::scriptedSessionInputs(confirmed)));
        }
    }

    ScriptedGame game;
    unison::session::Session session;
};

}

TEST_CASE("a spectator's session plays no frame the relay has not confirmed")
{
    Watched watched;

    watched.session.tick();

    REQUIRE(watched.session.predictedFrame() == 0U);
    REQUIRE(watched.session.isStalled());
}

TEST_CASE("a spectator's session plays every frame the relay confirmed, its predicted frame always its verified one")
{
    Watched watched;
    watched.confirmUpTo(3);

    for (int tick = 0; tick < 5; ++tick)
    {
        watched.session.tick();

        REQUIRE(watched.session.predictedFrame() == watched.session.verifiedFrame());
    }

    REQUIRE(watched.session.verifiedFrame() == 3U);
}

TEST_CASE("a spectator's session with a delay plays a frame only once the frames of the delay after it are confirmed")
{
    Watched watched{2};
    watched.confirmUpTo(3);

    watched.session.tick();
    watched.session.tick();

    REQUIRE(watched.session.predictedFrame() == 1U);
    REQUIRE(watched.session.verifiedFrame() == 1U);
}

TEST_CASE("a spectator's session verifies the checksums a player's session does")
{
    Watched watched;
    watched.confirmUpTo(4);

    for (int tick = 0; tick < 4; ++tick)
    {
        watched.session.tick();
    }

    const std::span<const unison::session::VerifiedChecksum> checksums = watched.session.verifiedChecksums();
    REQUIRE(checksums.size() == 4U);

    for (const unison::session::VerifiedChecksum& checksum : checksums)
    {
        CAPTURE(checksum.frameNumber);
        REQUIRE(checksum.checksum == unison::test::checksumOfScriptedSession(checksum.frameNumber));
    }
}
