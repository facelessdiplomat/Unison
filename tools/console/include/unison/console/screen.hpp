#pragma once

#include <unison/console/status_line.hpp>

#include <string>
#include <string_view>

namespace unison::console
{

/// What a console shows while it plays: the status line of its client above the map of the match.
[[nodiscard]] std::string screenOf(const ConsoleStatus& status, std::string_view map);

/// The terminal's escape sequences that draw a screen over the last one from the top left corner, each line
/// clearing what the last screen left beyond its end and the rest cleared below; no line breaks after the last.
[[nodiscard]] std::string redrawnInPlace(std::string_view screen);

}
