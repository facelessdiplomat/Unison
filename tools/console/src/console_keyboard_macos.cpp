#include "console_keyboard.hpp"

#include <unison/console/key_codes.hpp>

#include <unison/core/log_sink.hpp>

#include <CoreGraphics/CoreGraphics.h>
#include <termios.h>
#include <unistd.h>

#include <array>

namespace unison::console
{

namespace
{

constexpr std::size_t kBytesPerDrain = 64;

void drainTypedInput()
{
    std::array<char, kBytesPerDrain> typed{};

    while (read(STDIN_FILENO, typed.data(), typed.size()) > 0)
    {
    }
}

}

struct ConsoleKeyboard::Terminal
{
    termios original{};
};

ConsoleKeyboard::ConsoleKeyboard()
{
    termios original{};

    if (isatty(STDIN_FILENO) == 0 || tcgetattr(STDIN_FILENO, &original) != 0)
    {
        return;
    }

    termios keyAtATime = original;
    keyAtATime.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON);
    keyAtATime.c_cc[VMIN] = 0;
    keyAtATime.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &keyAtATime) != 0)
    {
        return;
    }

    terminal = std::make_unique<Terminal>(Terminal{original});
}

ConsoleKeyboard::~ConsoleKeyboard()
{
    if (terminal != nullptr)
    {
        static_cast<void>(tcflush(STDIN_FILENO, TCIFLUSH));
        static_cast<void>(tcsetattr(STDIN_FILENO, TCSANOW, &terminal->original));

        if (!CGPreflightListenEventAccess())
        {
            logMessage(LogLevel::Info,
                       "unison_console: this terminal holds no Input Monitoring; if the keys did nothing, allow it in "
                       "System Settings, Privacy & Security, Input Monitoring, then reopen the terminal");
        }
    }
}

bool ConsoleKeyboard::isAvailable() const
{
    return terminal != nullptr;
}

void ConsoleKeyboard::readInto(HeldKeys& keys)
{
    if (terminal == nullptr)
    {
        return;
    }

    drainTypedInput();

    for (const GameKey key : kGameKeys)
    {
        noteKey(keys, key, CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, macKeyCodeOf(key)));
    }
}

}
