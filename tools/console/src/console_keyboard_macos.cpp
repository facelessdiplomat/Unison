#include "console_keyboard.hpp"

namespace unison::console
{

struct ConsoleKeyboard::Terminal
{
};

ConsoleKeyboard::ConsoleKeyboard() = default;

ConsoleKeyboard::~ConsoleKeyboard() = default;

bool ConsoleKeyboard::isAvailable() const
{
    return terminal != nullptr;
}

void ConsoleKeyboard::readInto(HeldKeys&)
{
}

}
