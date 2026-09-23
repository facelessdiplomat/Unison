#pragma once

#include <unison/console/arena_controls.hpp>

namespace unison::console
{

class ConsoleKeyboard
{
public:
    ConsoleKeyboard();

    [[nodiscard]] bool isAvailable() const;

    void readInto(HeldKeys& keys);

private:
    void* input = nullptr;
    bool available = false;
};

}
