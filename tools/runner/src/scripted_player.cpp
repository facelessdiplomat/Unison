#include <unison/runner/scripted_player.hpp>

namespace unison::runner
{

namespace
{

constexpr std::int32_t kShortestCourse = 10;
constexpr std::int32_t kLongestCourse = 40;
constexpr std::int32_t kFullAxis = 127;
constexpr std::int32_t kTurnSteps = 65536;
constexpr std::int32_t kFramesPerJump = 30;
constexpr std::int32_t kFramesPerShot = 6;

std::uint64_t seedOfPlayer(std::uint64_t runSeed, std::uint32_t player)
{
    Rng source{runSeed};
    std::uint64_t drawn = 0;

    for (std::uint32_t skipped = 0; skipped <= player; ++skipped)
    {
        const std::uint64_t high = source.nextUint32();
        const std::uint64_t low = source.nextUint32();

        drawn = (high << 32U) | low;
    }

    return drawn;
}

std::int8_t axisDrawn(Rng& rng)
{
    return static_cast<std::int8_t>(rng.nextInRange(-1, 1) * kFullAxis);
}

bool isOneIn(Rng& rng, std::int32_t chances)
{
    return rng.nextInRange(1, chances) == 1;
}

}

ScriptedPlayer::ScriptedPlayer(std::uint64_t seed, std::uint32_t player) : rng{seedOfPlayer(seed, player)}
{
}

arena::ArenaInput ScriptedPlayer::nextInput()
{
    if (framesLeftOnCourse == 0)
    {
        course.moveX = axisDrawn(rng);
        course.moveY = axisDrawn(rng);
        course.yaw = static_cast<std::int16_t>(rng.nextInRange(-kTurnSteps / 2, kTurnSteps / 2 - 1));
        framesLeftOnCourse = rng.nextInRange(kShortestCourse, kLongestCourse);
    }

    --framesLeftOnCourse;

    arena::ArenaInput input = course;

    if (isOneIn(rng, kFramesPerJump))
    {
        input.buttons |= static_cast<std::uint16_t>(arena::Button::Jump);
    }

    if (isOneIn(rng, kFramesPerShot))
    {
        input.buttons |= static_cast<std::uint16_t>(arena::Button::Fire);
    }

    return input;
}

}
