#include <unison/console/screen.hpp>

namespace unison::console
{

namespace
{

constexpr std::string_view kCursorHome = "\x1b[H";
constexpr std::string_view kClearToLineEnd = "\x1b[K";
constexpr std::string_view kClearBelow = "\x1b[J";

}

std::string screenOf(const ConsoleStatus& status, std::string_view map)
{
    return statusLineOf(status) + '\n' + std::string{map};
}

std::string redrawnInPlace(std::string_view screen)
{
    const std::string_view lines = screen.ends_with('\n') ? screen.substr(0, screen.size() - 1) : screen;
    std::string drawn{kCursorHome};

    for (const char character : lines)
    {
        if (character == '\n')
        {
            drawn += kClearToLineEnd;
        }

        drawn += character;
    }

    drawn += kClearToLineEnd;
    drawn += kClearBelow;

    return drawn;
}

}
