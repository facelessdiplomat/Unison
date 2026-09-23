#include "console_screen.hpp"

#include <unison/console/screen.hpp>

#include <windows.h>

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

ConsoleScreen::ConsoleScreen() : output{GetStdHandle(STD_OUTPUT_HANDLE)}
{
    DWORD mode = 0;

    available = output != nullptr && output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode) != 0 &&
                SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    originalMode = mode;

    if (available)
    {
        write(kHideCursor);
    }
}

ConsoleScreen::~ConsoleScreen()
{
    if (available)
    {
        write(kBelowTheScreenWithTheCursorShown);
        static_cast<void>(SetConsoleMode(output, originalMode));
    }
}

bool ConsoleScreen::isAvailable() const
{
    return available;
}

void ConsoleScreen::show(std::string_view screen)
{
    if (available)
    {
        write(redrawnInPlace(screen));
    }
}

}
