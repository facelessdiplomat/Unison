#include <unison/console/status_line.hpp>

#include <format>

namespace unison::console
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerMillisecond = 1'000;

std::string inMatch(const ConsoleStatus& status, std::string_view standing)
{
    return std::format("unison_console: {} {} in slot {}, verified {}, predicted {}, rollbacks in the last second {}, "
                       "round trip {} ms",
                       status.name,
                       standing,
                       status.slot,
                       status.verifiedFrame,
                       status.predictedFrame,
                       status.rollbacksLastSecond,
                       status.roundTripMicroseconds / kMicrosecondsPerMillisecond);
}

}

std::string statusLineOf(const ConsoleStatus& status)
{
    using session::ConnectionState;

    if (status.state == ConnectionState::Idle)
    {
        return std::format("unison_console: {} idle", status.name);
    }

    if (status.state == ConnectionState::Connecting)
    {
        return std::format("unison_console: {} connecting to the relay", status.name);
    }

    if (status.state == ConnectionState::Joining)
    {
        return std::format("unison_console: {} joining the match", status.name);
    }

    if (status.state == ConnectionState::Disconnected)
    {
        return std::format("unison_console: {} disconnected", status.name);
    }

    return inMatch(status, status.state == ConnectionState::Stalled ? "stalled" : "playing");
}

}
