#pragma once

#include <memory>

namespace unison::console
{

class TerminalOutput
{
public:
    TerminalOutput();
    ~TerminalOutput();

    TerminalOutput(const TerminalOutput&) = delete;
    TerminalOutput& operator=(const TerminalOutput&) = delete;
    TerminalOutput(TerminalOutput&&) = delete;
    TerminalOutput& operator=(TerminalOutput&&) = delete;

    [[nodiscard]] bool isAvailable() const;

private:
    struct Mode;

    std::unique_ptr<Mode> mode;
};

}
