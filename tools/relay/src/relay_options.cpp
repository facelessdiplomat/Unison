#include <unison/relay/relay_options.hpp>

#include <cxxopts.hpp>

#include <string_view>

namespace unison::relay
{

namespace
{

cxxopts::Options describedOptions()
{
    cxxopts::Options options{"unison_relay",
                             "Relays the inputs of the matches its clients play, one room for every match, and never "
                             "simulates them."};

    cxxopts::OptionAdder add = options.add_options();

    add("bind", "IPv4 address to listen on", cxxopts::value<std::string>()->default_value("0.0.0.0"));
    add("port",
        "port to listen on, 0 for one the system picks",
        cxxopts::value<std::uint16_t>()->default_value("7777"));
    add("max-peers", "peers connected at once", cxxopts::value<std::uint32_t>()->default_value("64"));
    add("input-deadline", "wait for a missing input, in ms", cxxopts::value<std::uint32_t>()->default_value("100"));
    add("resend-interval", "frames between reliable resends", cxxopts::value<std::uint32_t>()->default_value("10"));
    add("peer-timeout", "silence before a peer is gone, in ms", cxxopts::value<std::uint32_t>()->default_value("5000"));
    add("run-for", "seconds to run, 0 until stopped", cxxopts::value<std::uint32_t>()->default_value("0"));
    add("help", "lists these options");

    return options;
}

tl::expected<RelayOptions, Error> refused(std::string_view why)
{
    return tl::unexpected{Error{ErrorCode::InvalidOption, why}};
}

}

tl::expected<RelayOptions, Error> parseRelayOptions(std::span<const char* const> arguments)
{
    cxxopts::Options options = describedOptions();
    const cxxopts::ParseResult parsed = options.parse(static_cast<int>(arguments.size()), arguments.data());

    RelayOptions read;
    read.bindAddress = parsed["bind"].as<std::string>();
    read.port = parsed["port"].as<std::uint16_t>();
    read.maxPeers = parsed["max-peers"].as<std::uint32_t>();
    read.inputDeadlineMilliseconds = parsed["input-deadline"].as<std::uint32_t>();
    read.reliableResendInterval = parsed["resend-interval"].as<std::uint32_t>();
    read.peerTimeoutMilliseconds = parsed["peer-timeout"].as<std::uint32_t>();
    read.runForSeconds = parsed["run-for"].as<std::uint32_t>();
    read.isHelpAsked = parsed.count("help") > 0;

    if (read.maxPeers == 0)
    {
        return refused("--max-peers takes one peer at least");
    }

    if (read.inputDeadlineMilliseconds == 0)
    {
        return refused("--input-deadline takes one millisecond at least");
    }

    if (read.reliableResendInterval == 0)
    {
        return refused("--resend-interval takes one frame at least");
    }

    if (read.peerTimeoutMilliseconds == 0)
    {
        return refused("--peer-timeout takes one millisecond at least");
    }

    return read;
}

std::string relayHelp()
{
    return describedOptions().help();
}

}
