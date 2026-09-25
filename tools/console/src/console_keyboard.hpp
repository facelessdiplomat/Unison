#pragma once

#include <unison/console/arena_controls.hpp>

#include <memory>

namespace unison::console
{

class ConsoleKeyboard
{
public:
    ConsoleKeyboard();
    ~ConsoleKeyboard();

    ConsoleKeyboard(const ConsoleKeyboard&) = delete;
    ConsoleKeyboard& operator=(const ConsoleKeyboard&) = delete;
    ConsoleKeyboard(ConsoleKeyboard&&) = delete;
    ConsoleKeyboard& operator=(ConsoleKeyboard&&) = delete;

    [[nodiscard]] bool isAvailable() const;

    void readInto(HeldKeys& keys);

private:
    struct Terminal;

    std::unique_ptr<Terminal> terminal;
};

}
