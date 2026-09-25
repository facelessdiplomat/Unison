#include "console_screen.hpp"

#include <unison/console/screen.hpp>

#include <cstdio>

namespace unison::console
{

namespace
{

constexpr std::string_view kHideCursor = "\x1b[?25l";
constexpr std::string_view kBelowTheScreenWithTheCursorShown = "\n\x1b[?25h";

void write(std::string_view text)
{
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fflush(stdout);
}

}

ConsoleScreen::ConsoleScreen()
{
    if (terminal.isAvailable())
    {
        write(kHideCursor);
    }
}

ConsoleScreen::~ConsoleScreen()
{
    if (terminal.isAvailable())
    {
        write(kBelowTheScreenWithTheCursorShown);
    }
}

bool ConsoleScreen::isAvailable() const
{
    return terminal.isAvailable();
}

void ConsoleScreen::show(std::string_view screen)
{
    if (terminal.isAvailable())
    {
        write(redrawnInPlace(screen));
    }
}

}
