#include "terminal_output.hpp"

#include <windows.h>

namespace unison::console
{

struct TerminalOutput::Mode
{
    HANDLE output = nullptr;
    DWORD original = 0;
};

TerminalOutput::TerminalOutput()
{
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD original = 0;

    if (output != nullptr && output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &original) != 0 &&
        SetConsoleMode(output, original | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0)
    {
        mode = std::make_unique<Mode>(Mode{output, original});
    }
}

TerminalOutput::~TerminalOutput()
{
    if (mode != nullptr)
    {
        static_cast<void>(SetConsoleMode(mode->output, mode->original));
    }
}

bool TerminalOutput::isAvailable() const
{
    return mode != nullptr;
}

}
