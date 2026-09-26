#include <unison/console/console_options.hpp>

#include <unison/net/protocol.hpp>

#include <cxxopts.hpp>

#include <string_view>

namespace unison::console
{

namespace
{

cxxopts::Options describedOptions()
{
    cxxopts::Options options{"unison_console", "Plays the arena through a relay and prints how it goes."};

    cxxopts::OptionAdder add = options.add_options();

    add("host", "IPv4 address of the relay", cxxopts::value<std::string>()->default_value("127.0.0.1"));
    add("port", "port the relay listens on", cxxopts::value<std::uint16_t>()->default_value("7777"));
    add("from", "IPv4 address to send from, any when empty", cxxopts::value<std::string>()->default_value(""));
    add("name", "name this console shows", cxxopts::value<std::string>()->default_value("player"));
    add("players", "players in the match, the same for all", cxxopts::value<std::uint32_t>()->default_value("2"));
    add("run-for", "seconds to run, 0 until stopped", cxxopts::value<std::uint32_t>()->default_value("0"));
    add("record", "file the match is recorded into", cxxopts::value<std::string>()->default_value(""));
    add("help", "lists these options");

    return options;
}

}

tl::expected<ConsoleOptions, Error> parseConsoleOptions(std::span<const char* const> arguments)
{
    cxxopts::Options options = describedOptions();
    const cxxopts::ParseResult parsed = options.parse(static_cast<int>(arguments.size()), arguments.data());

    ConsoleOptions read;
    read.host = parsed["host"].as<std::string>();
    read.port = parsed["port"].as<std::uint16_t>();
    read.from = parsed["from"].as<std::string>();
    read.name = parsed["name"].as<std::string>();
    read.players = parsed["players"].as<std::uint32_t>();
    read.runForSeconds = parsed["run-for"].as<std::uint32_t>();
    read.recordPath = parsed["record"].as<std::string>();
    read.isHelpAsked = parsed.count("help") > 0;

    if (read.players == 0 || read.players > net::kMaxSlots)
    {
        return tl::unexpected{Error{ErrorCode::InvalidOption, "--players takes one player at least and eight at most"}};
    }

    return read;
}

std::string consoleHelp()
{
    return describedOptions().help();
}

}
