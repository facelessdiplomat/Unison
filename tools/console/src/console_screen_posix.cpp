#include "console_screen.hpp"

namespace unison::console
{

struct ConsoleScreen::Terminal
{
};

ConsoleScreen::ConsoleScreen() = default;

ConsoleScreen::~ConsoleScreen() = default;

bool ConsoleScreen::isAvailable() const
{
    return terminal != nullptr;
}

void ConsoleScreen::show(std::string_view)
{
}

}
