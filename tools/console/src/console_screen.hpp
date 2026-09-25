#pragma once

#include <memory>
#include <string_view>

namespace unison::console
{

class ConsoleScreen
{
public:
    ConsoleScreen();
    ~ConsoleScreen();

    ConsoleScreen(const ConsoleScreen&) = delete;
    ConsoleScreen& operator=(const ConsoleScreen&) = delete;
    ConsoleScreen(ConsoleScreen&&) = delete;
    ConsoleScreen& operator=(ConsoleScreen&&) = delete;

    [[nodiscard]] bool isAvailable() const;

    void show(std::string_view screen);

private:
    struct Terminal;

    std::unique_ptr<Terminal> terminal;
};

}
