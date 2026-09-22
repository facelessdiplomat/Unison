#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <arena/arena_input.hpp>
#include <arena/components.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/pipeline_hash.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

constexpr std::size_t kPlayers = 4;
constexpr std::uint32_t kFrames = 600;
constexpr std::uint64_t kGoldenArenaChecksum = 0x05E94D8E12D2D6BCU;

unison::sim::FrameInputs scripted(std::uint32_t frameNumber)
{
    unison::sim::FrameInputs inputs;

    for (std::uint32_t slot = 0; slot < kPlayers; ++slot)
    {
        const std::uint32_t beat = frameNumber / 24U + slot;

        arena::ArenaInput input;
        input.moveY = static_cast<std::int8_t>((static_cast<std::int32_t>(beat % 3U) - 1) * 127);
        input.moveX = static_cast<std::int8_t>((static_cast<std::int32_t>((beat / 3U) % 3U) - 1) * 127);
        input.yaw =
            static_cast<std::int16_t>(static_cast<std::int32_t>((frameNumber * (slot + 1U) * 211U) % 65536U) - 32768);

        if ((frameNumber + slot * 7U) % 31U == 0U)
        {
            input.buttons |= static_cast<std::uint16_t>(arena::Button::Jump);
        }

        if ((frameNumber + slot * 5U) % 9U == 0U)
        {
            input.buttons |= static_cast<std::uint16_t>(arena::Button::Fire);
        }

        inputs.set(slot, input, unison::sim::InputFlags::Present);
    }

    return inputs;
}

void play(arena::ArenaSimulation& match, std::uint32_t frames)
{
    for (std::uint32_t frameNumber = 0; frameNumber < frames; ++frameNumber)
    {
        match.advance(scripted(frameNumber));
    }
}

}

TEST_CASE("a match is set up with its world, its players and its systems in order")
{
    const arena::ArenaSimulation match{kPlayers};

    REQUIRE(match.pipeline().size() == 8U);
    REQUIRE(match.pipeline().at(0).name() == "ApplyInput");
    REQUIRE(match.pipeline().at(3).name() == "PhysicsStep");
    REQUIRE(match.pipeline().at(7).name() == "MatchRules");
    REQUIRE(match.frame().registry.view<const arena::PlayerSlot>().size() == kPlayers);
    REQUIRE(match.frame().physics.bodyCount() > kPlayers);
    REQUIRE(match.assets().isFrozen());
}

TEST_CASE("the players of a match stand where the match put them")
{
    const arena::ArenaSimulation match{kPlayers};

    for (const auto [entity, slot, stance] :
         match.frame().registry.view<const arena::PlayerSlot, const unison::sim::Transform>().each())
    {
        REQUIRE(stance.position.y > 0.0F);
        REQUIRE(match.frame().registry.all_of<unison::sim::CharacterController>(entity));
    }
}

TEST_CASE("a match played twice on the same inputs agrees on every frame")
{
    arena::ArenaSimulation one{kPlayers};
    arena::ArenaSimulation other{kPlayers};

    for (std::uint32_t frameNumber = 0; frameNumber < kFrames; ++frameNumber)
    {
        one.advance(scripted(frameNumber));
        other.advance(scripted(frameNumber));

        if (unison::sim::checksumOf(one.frame()) != unison::sim::checksumOf(other.frame()))
        {
            FAIL("the matches parted ways on frame " << frameNumber);
        }
    }

    REQUIRE(unison::sim::checksumOf(one.frame()) == unison::sim::checksumOf(other.frame()));
}

TEST_CASE("a match played on the scripted inputs ends where it always has")
{
    arena::ArenaSimulation match{kPlayers};

    play(match, kFrames);

    REQUIRE(unison::sim::checksumOf(match.frame()) == kGoldenArenaChecksum);
}

TEST_CASE("the scripted match is a match worth checking")
{
    arena::ArenaSimulation match{kPlayers};

    play(match, kFrames);

    std::uint32_t kills = 0;

    for (const auto [entity, score] : match.frame().registry.view<const arena::Score>().each())
    {
        kills += score.kills;
    }

    REQUIRE(match.frame().globals.matchPhase != unison::sim::MatchPhase::Warmup);
    REQUIRE(kills > 0U);
}
