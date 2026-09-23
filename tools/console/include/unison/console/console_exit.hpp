#pragma once

#include <unison/console/status_line.hpp>

namespace unison::console
{

/// The exit code a console ends with: two when the relay found it out of step with the others, whatever else
/// happened, one when it lost the relay, nought otherwise.
[[nodiscard]] int exitCodeOf(const ConsoleStatus& status);

}
