#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <unison/session/session.hpp>
#include <unison/sim/frame_checksum.hpp>

#include <cstdint>

namespace
{

constexpr std::uint32_t kFrames = 200;
constexpr std::uint32_t kConfirmationDelay = 3;
constexpr std::size_t kLocalSlot = 0;

unison::net::SessionConfig scriptedMatch()
{
    unison::net::SessionConfig config;
    config.slotCount = static_cast<std::uint8_t>(unison::test::kScriptedPlayers);
    config.inputSize = sizeof(arena::ArenaInput);
    config.maxPrediction = 8;

    return config;
}

unison::sim::FrameInputs inputsOfFrame(std::uint32_t frameNumber)
{
    return unison::test::scriptedInputs(frameNumber - 1);
}

std::uint64_t checksumOfScriptedMatch(std::uint32_t frames)
{
    arena::ArenaSimulation match{unison::test::kScriptedPlayers};

    while (match.frame().frameNumber < frames)
    {
        match.advance(unison::test::scriptedInputs(match.frame().frameNumber));
    }

    return unison::sim::checksumOf(match.frame());
}

}

TEST_CASE("a match played through a session that keeps guessing wrong ends where the scripted match does")
{
    arena::ArenaSimulation match{unison::test::kScriptedPlayers};
    unison::session::Session session{match.frame(), match.pipeline(), scriptedMatch(), kLocalSlot};

    for (std::uint32_t next = 1; next <= kFrames; ++next)
    {
        const unison::sim::FrameInputs scripted = inputsOfFrame(next);
        session.setLocalInput(scripted.bytesAt(kLocalSlot));
        session.tick();

        if (next > kConfirmationDelay)
        {
            REQUIRE(session.confirm(next - kConfirmationDelay, inputsOfFrame(next - kConfirmationDelay)));
        }
    }

    for (std::uint32_t late = kFrames - kConfirmationDelay + 1; late <= kFrames + 1; ++late)
    {
        REQUIRE(session.confirm(late, inputsOfFrame(late)));
    }

    session.tick();

    REQUIRE(session.verifiedFrame() == kFrames + 1);
    REQUIRE(unison::sim::checksumOf(match.frame()) == checksumOfScriptedMatch(kFrames + 1));
}
