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

struct ConsoleScreen::Terminal
{
    HANDLE output = nullptr;
    DWORD originalMode = 0;
};

ConsoleScreen::ConsoleScreen()
{
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (output != nullptr && output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode) != 0 &&
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0)
    {
        terminal = std::make_unique<Terminal>(Terminal{output, mode});
        write(kHideCursor);
    }
}

ConsoleScreen::~ConsoleScreen()
{
    if (terminal != nullptr)
    {
        write(kBelowTheScreenWithTheCursorShown);
        static_cast<void>(SetConsoleMode(terminal->output, terminal->originalMode));
    }
}

bool ConsoleScreen::isAvailable() const
{
    return terminal != nullptr;
}

void ConsoleScreen::show(std::string_view screen)
{
    if (terminal != nullptr)
    {
        write(redrawnInPlace(screen));
    }
}

}
