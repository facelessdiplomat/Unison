#include <unison/console/arena_controls.hpp>

namespace unison::console
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;
constexpr std::uint64_t kWholeTurnStepMicroseconds = 65'536ULL * kMicrosecondsPerSecond;
constexpr std::int8_t kFullAxis = 127;

std::int8_t axisOf(bool towards, bool away)
{
    return static_cast<std::int8_t>((towards ? kFullAxis : 0) - (away ? kFullAxis : 0));
}

}

arena::ArenaInput ArenaControls::inputFor(const HeldKeys& keys, std::uint64_t heldMicroseconds)
{
    const std::uint64_t turned = heldMicroseconds * kAimStepsPerSecond % kWholeTurnStepMicroseconds;

    if (keys.turnLeft && !keys.turnRight)
    {
        aimStepMicroseconds = (aimStepMicroseconds + turned) % kWholeTurnStepMicroseconds;
    }

    if (keys.turnRight && !keys.turnLeft)
    {
        aimStepMicroseconds = (aimStepMicroseconds + kWholeTurnStepMicroseconds - turned) % kWholeTurnStepMicroseconds;
    }

    arena::ArenaInput input;
    input.moveX = axisOf(keys.right, keys.left);
    input.moveY = axisOf(keys.forward, keys.back);
    input.yaw = static_cast<std::int16_t>(static_cast<std::uint16_t>(aimStepMicroseconds / kMicrosecondsPerSecond));

    if (keys.jump)
    {
        input.buttons |= static_cast<std::uint16_t>(arena::Button::Jump);
    }

    if (keys.fire)
    {
        input.buttons |= static_cast<std::uint16_t>(arena::Button::Fire);
    }

    return input;
}

void noteKey(HeldKeys& keys, GameKey key, bool isDown)
{
    switch (key)
    {
        case GameKey::Forward:
            keys.forward = isDown;
            break;
        case GameKey::Back:
            keys.back = isDown;
            break;
        case GameKey::Left:
            keys.left = isDown;
            break;
        case GameKey::Right:
            keys.right = isDown;
            break;
        case GameKey::Jump:
            keys.jump = isDown;
            break;
        case GameKey::Fire:
            keys.fire = isDown;
            break;
        case GameKey::TurnLeft:
            keys.turnLeft = isDown;
            break;
        case GameKey::TurnRight:
            keys.turnRight = isDown;
            break;
    }
}

}
