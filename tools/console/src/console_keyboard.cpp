#include "console_keyboard.hpp"

#include <windows.h>

#include <algorithm>
#include <array>

namespace unison::console
{

namespace
{

constexpr DWORD kRecordsPerRead = 16;

}

ConsoleKeyboard::ConsoleKeyboard() : input{GetStdHandle(STD_INPUT_HANDLE)}
{
    DWORD mode = 0;
    available = input != nullptr && input != INVALID_HANDLE_VALUE && GetConsoleMode(input, &mode) != 0;
}

bool ConsoleKeyboard::isAvailable() const
{
    return available;
}

void ConsoleKeyboard::readInto(HeldKeys& keys)
{
    if (!available)
    {
        return;
    }

    DWORD waiting = 0;

    if (GetNumberOfConsoleInputEvents(input, &waiting) == 0)
    {
        return;
    }

    std::array<INPUT_RECORD, kRecordsPerRead> records{};

    while (waiting > 0)
    {
        DWORD read = 0;

        if (ReadConsoleInputW(input, records.data(), std::min(waiting, kRecordsPerRead), &read) == 0 || read == 0)
        {
            return;
        }

        for (DWORD index = 0; index < read; ++index)
        {
            const INPUT_RECORD& record = records[index];

            if (record.EventType == KEY_EVENT)
            {
                noteKey(keys, record.Event.KeyEvent.wVirtualKeyCode, record.Event.KeyEvent.bKeyDown != FALSE);
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
