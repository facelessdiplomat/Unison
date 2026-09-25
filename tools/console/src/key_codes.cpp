#include <unison/console/key_codes.hpp>

namespace unison::console
{

namespace
{

constexpr std::uint16_t kWindowsSpaceKey = 0x20;

}

std::optional<GameKey> gameKeyOfWindowsKey(std::uint16_t virtualKey)
{
    switch (virtualKey)
    {
        case 'W':
            return GameKey::Forward;
        case 'S':
            return GameKey::Back;
        case 'A':
            return GameKey::Left;
        case 'D':
            return GameKey::Right;
        case kWindowsSpaceKey:
            return GameKey::Jump;
        case 'F':
            return GameKey::Fire;
        case 'Q':
            return GameKey::TurnLeft;
        case 'E':
            return GameKey::TurnRight;
        default:
            return std::nullopt;
    }
}

}
