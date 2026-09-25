#pragma once

#include <arena/arena_input.hpp>

#include <array>
#include <cstdint>

namespace unison::console
{

/// The keys a console player plays the arena with, whatever the platform calls them.
enum class GameKey : std::uint8_t
{
    Forward,
    Back,
    Left,
    Right,
    Jump,
    Fire,
    TurnLeft,
    TurnRight
};

/// Every game key, in the order `GameKey` names them.
inline constexpr std::array<GameKey, 8> kGameKeys{GameKey::Forward,
                                                  GameKey::Back,
                                                  GameKey::Left,
                                                  GameKey::Right,
                                                  GameKey::Jump,
                                                  GameKey::Fire,
                                                  GameKey::TurnLeft,
                                                  GameKey::TurnRight};

/// The game keys a console player holds down: W and S, A and D, Space, F, and Q and E.
struct HeldKeys
{
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool jump = false;
    bool fire = false;
    bool turnLeft = false;
    bool turnRight = false;
};

/// How fast Q and E turn the aim: half a turn a second, in the 65536 steps of a whole one.
inline constexpr std::uint64_t kAimStepsPerSecond = 32'768;

/// Turns the keys a console player holds into the arena's input: W and S move forward and back, A and D to
/// the sides, Space jumps, F fires, and Q and E turn the aim left and right for as long as they are held,
/// by the time they were held rather than by how often the input is asked for.
class ArenaControls
{
public:
    /// The input for the keys held now, the aim turned for the microseconds since the last input.
    [[nodiscard]] arena::ArenaInput inputFor(const HeldKeys& keys, std::uint64_t heldMicroseconds);

private:
    std::uint64_t aimStepMicroseconds = 0;
};

/// Notes a game key going down or up.
void noteKey(HeldKeys& keys, GameKey key, bool isDown);

}
