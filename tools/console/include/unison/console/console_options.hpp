#pragma once

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <span>
#include <string>

namespace unison::console
{

/// What one console plays: the relay it joins at an address and port, the address it sends from, any the
/// system picks when empty, the name it shows, how many players the match has, which every console of the
/// match must agree on, for how many seconds it runs, nought for until it is stopped, the file it records the match
/// into, none when empty, and the folder it dumps the snapshot of a desync into, none when empty. A console asked for
/// help lists the options instead.
struct ConsoleOptions
{
    std::string host = "127.0.0.1";
    std::uint16_t port = 7777;
    std::string from;
    std::string name = "player";
    std::uint32_t players = 2;
    std::uint32_t runForSeconds = 0;
    std::string recordPath;
    std::string dumpDirectory = ".";
    bool isHelpAsked = false;
};

/// Reads the console's command line, the program's name first. Players outside one to eight are refused with an
/// error naming the option. An unknown option or a value that is no number ends the program with the message
/// the parser prints.
[[nodiscard]] tl::expected<ConsoleOptions, Error> parseConsoleOptions(std::span<const char* const> arguments);

/// Every option of the console and what it does, as `--help` prints them.
[[nodiscard]] std::string consoleHelp();

}
