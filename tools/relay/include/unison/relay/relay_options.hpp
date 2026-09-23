#pragma once

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <span>
#include <string>

namespace unison::relay
{

/// How a standalone relay runs: the address and port it listens on, how many peers it takes at once, how long
/// it waits for a missing input and every how many frames it sends confirmations again reliably, how long a
/// peer may stay silent before it is taken for gone, and for how many seconds it runs, nought for until it is
/// stopped. A relay asked for help lists the options instead.
struct RelayOptions
{
    std::string bindAddress = "0.0.0.0";
    std::uint16_t port = 7777;
    std::uint32_t maxPeers = 64;
    std::uint32_t inputDeadlineMilliseconds = 100;
    std::uint32_t reliableResendInterval = 10;
    std::uint32_t peerTimeoutMilliseconds = 5000;
    std::uint32_t runForSeconds = 0;
    bool isHelpAsked = false;
};

/// Reads the relay's command line, the program's name first. A value no relay can run with is refused with an
/// error naming the option: no peers, and a deadline, a resend interval or a peer timeout of nought. An
/// unknown option or a value that is no number ends the program with the message the parser prints.
[[nodiscard]] tl::expected<RelayOptions, Error> parseRelayOptions(std::span<const char* const> arguments);

/// Every option of the relay and what it does, as `--help` prints them.
[[nodiscard]] std::string relayHelp();

}
