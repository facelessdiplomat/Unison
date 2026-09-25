#include "console_keyboard.hpp"

#include <unison/console/key_codes.hpp>

#include <windows.h>

#include <algorithm>
#include <array>
#include <optional>

namespace unison::console
{

namespace
{

constexpr DWORD kRecordsPerRead = 16;

bool isConsoleInput(HANDLE input)
{
    DWORD mode = 0;

    return input != nullptr && input != INVALID_HANDLE_VALUE && GetConsoleMode(input, &mode) != 0;
}

}

struct ConsoleKeyboard::Terminal
{
    HANDLE input = nullptr;
};

ConsoleKeyboard::ConsoleKeyboard()
{
    const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);

    if (isConsoleInput(input))
    {
        terminal = std::make_unique<Terminal>(Terminal{input});
    }
}

ConsoleKeyboard::~ConsoleKeyboard() = default;

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

    DWORD waiting = 0;

    if (GetNumberOfConsoleInputEvents(terminal->input, &waiting) == 0)
    {
        return;
    }

    std::array<INPUT_RECORD, kRecordsPerRead> records{};

    while (waiting > 0)
    {
        DWORD read = 0;

        if (ReadConsoleInputW(terminal->input, records.data(), std::min(waiting, kRecordsPerRead), &read) == 0 ||
            read == 0)
        {
            return;
        }

        for (DWORD index = 0; index < read; ++index)
        {
            const INPUT_RECORD& record = records[index];

            if (record.EventType == KEY_EVENT)
            {
                if (const std::optional<GameKey> key = gameKeyOfWindowsKey(record.Event.KeyEvent.wVirtualKeyCode))
                {
                    noteKey(keys, *key, record.Event.KeyEvent.bKeyDown != FALSE);
                }
            }

            if (record.EventType == FOCUS_EVENT && record.Event.FocusEvent.bSetFocus == FALSE)
            {
                keys = HeldKeys{};
            }
        }

        waiting -= read;
    }
}

}
