#include "terminal_output.hpp"

#include <unistd.h>

namespace unison::console
{

struct TerminalOutput::Mode
{
};

TerminalOutput::TerminalOutput()
{
    if (isatty(STDOUT_FILENO) != 0)
    {
        mode = std::make_unique<Mode>();
    }
}

TerminalOutput::~TerminalOutput() = default;

bool TerminalOutput::isAvailable() const
{
    return mode != nullptr;
}

}
