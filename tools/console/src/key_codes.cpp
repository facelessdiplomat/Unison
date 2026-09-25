#include <unison/console/key_codes.hpp>

#include <unison/core/contract.hpp>

namespace unison::console
{

namespace
{

constexpr std::uint16_t kWindowsSpaceKey = 0x20;
constexpr std::uint16_t kMacW = 0x0D;
constexpr std::uint16_t kMacS = 0x01;
constexpr std::uint16_t kMacA = 0x00;
constexpr std::uint16_t kMacD = 0x02;
constexpr std::uint16_t kMacSpace = 0x31;
constexpr std::uint16_t kMacF = 0x03;
constexpr std::uint16_t kMacQ = 0x0C;
constexpr std::uint16_t kMacE = 0x0E;

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

std::uint16_t macKeyCodeOf(GameKey key)
{
    switch (key)
    {
        case GameKey::Forward:
            return kMacW;
        case GameKey::Back:
            return kMacS;
        case GameKey::Left:
            return kMacA;
        case GameKey::Right:
            return kMacD;
        case GameKey::Jump:
            return kMacSpace;
        case GameKey::Fire:
            return kMacF;
        case GameKey::TurnLeft:
            return kMacQ;
        case GameKey::TurnRight:
            return kMacE;
    }

    UNISON_VERIFY(false);

    return kMacW;
}

}
