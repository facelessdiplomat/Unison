#pragma once

#include <unison/console/arena_controls.hpp>

#include <cstdint>
#include <optional>

namespace unison::console
{

/// The game key a Windows virtual-key code stands for, or nothing for a key the game does not use.
[[nodiscard]] std::optional<GameKey> gameKeyOfWindowsKey(std::uint16_t virtualKey);

}
