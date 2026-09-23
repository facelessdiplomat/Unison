#pragma once

#include <unison/sim/frame.hpp>

#include <cstddef>
#include <string>

namespace arena
{

/// How many columns and rows the text map of the arena has: half a metre a column and a metre a row, since a
/// character of a console is about twice as tall as it is wide.
inline constexpr std::size_t kMapColumns = 48;
inline constexpr std::size_t kMapRows = 24;

/// The arena seen from above as text, +X to the right and +Z down: '#' walls, '=' ramps, 'C' crates, '*' shots,
/// a player by the number of their slot with an arrow the way they face, '.' open floor. Below it a line for
/// every player in slot order gives their health, or that they wait to come back, and their kills.
[[nodiscard]] std::string textMapOf(const unison::sim::Frame& frame);

}
