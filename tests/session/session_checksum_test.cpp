#include <unison/session/session.hpp>

#include <support/fatal_handler_probe.hpp>
#include <support/session_script.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kFrames = 40;
constexpr std::uint32_t kConfirmationDelay = 3;

unison::net::SessionConfig scriptedSession(std::uint32_t checksumInterval)
{
    unison::net::SessionConfig config;
    config.slotCount = unison::test::kSessionSlots;
    config.inputSize = sizeof(unison::test::SampleInput);
    config.maxPrediction = 8;
    config.checksumInterval = checksumInterval;

    return config;
}

void takeChecksums(unison::session::Session& session, std::vector<unison::session::VerifiedChecksum>& taken)
{
    for (const unison::session::VerifiedChecksum& checksum : session.verifiedChecksums())
    {
        taken.push_back(checksum);
    }

    session.clearVerifiedChecksums();
}

std::vector<unison::session::VerifiedChecksum> checksumsOfScriptedSession(std::uint32_t checksumInterval)
{
    unison::sim::Frame frame;
    unison::test::addScoredEntity(frame);
    unison::test::InputMixer mixer;
    unison::sim::SystemPipeline pipeline;
    pipeline.add(mixer);
    unison::session::Session session{
        frame, pipeline, scriptedSession(checksumInterval), unison::test::kSessionLocalSlot};
    std::vector<unison::session::VerifiedChecksum> taken;

    for (std::uint32_t next = 1; next <= kFrames; ++next)
    {
        const unison::test::SampleInput local =
            unison::test::scriptedSessionInput(next, unison::test::kSessionLocalSlot);
        session.setLocalInput(unison::test::bytesOf(local));
        session.tick();

        if (next > kConfirmationDelay)
        {
            REQUIRE(session.confirm(next - kConfirmationDelay,
                                    unison::test::scriptedSessionInputs(next - kConfirmationDelay)));
        }

        takeChecksums(session, taken);
    }

    session.tick();
    takeChecksums(session, taken);

    return taken;
}

}

TEST_CASE("a verified frame hands out the checksum a match played straight through to it has")
{
    const std::vector<unison::session::VerifiedChecksum> taken = checksumsOfScriptedSession(1);

    REQUIRE(taken.size() == kFrames - kConfirmationDelay);

    for (std::uint32_t index = 0; index < taken.size(); ++index)
    {
        CAPTURE(index);

        REQUIRE(taken[index].frameNumber == index + 1);
        REQUIRE(taken[index].checksum == unison::test::checksumOfScriptedSession(index + 1));
    }
}

TEST_CASE("checksums are handed out only for the frames on the config's interval")
{
    const std::vector<unison::session::VerifiedChecksum> taken = checksumsOfScriptedSession(5);

    REQUIRE(taken.size() == (kFrames - kConfirmationDelay) / 5);

    for (std::uint32_t index = 0; index < taken.size(); ++index)
    {
        REQUIRE(taken[index].frameNumber == (index + 1) * 5);
    }
}

TEST_CASE("checksums once cleared are not handed out again")
{
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;
    unison::session::Session session{frame, pipeline, scriptedSession(1), unison::test::kSessionLocalSlot};
    session.tick();
    REQUIRE(unison::test::confirmAsPlayed(session, 1));
    session.clearVerifiedChecksums();
    session.tick();

    REQUIRE(unison::test::confirmAsPlayed(session, 2));

    REQUIRE(session.verifiedChecksums().size() == 1U);
    REQUIRE(session.verifiedChecksums()[0].frameNumber == 2U);
}

TEST_CASE("a session with no checksum interval breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::Frame frame;
    const unison::sim::SystemPipeline pipeline;

    const unison::session::Session session{frame, pipeline, scriptedSession(0), unison::test::kSessionLocalSlot};

    REQUIRE(probe.failureCount() == 1U);
}
