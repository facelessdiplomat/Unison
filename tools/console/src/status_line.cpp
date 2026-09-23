#include <unison/console/status_line.hpp>

#include <format>

namespace unison::console
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerMillisecond = 1'000;

std::string inMatch(const ConsoleStatus& status, std::string_view standing)
{
    return std::format("{} {} in slot {}, verified {}, predicted {}, rollbacks in the last second {}, "
                       "round trip {} ms, lead {} ms",
                       status.name,
                       standing,
                       status.slot,
                       status.verifiedFrame,
                       status.predictedFrame,
                       status.rollbacksLastSecond,
                       status.roundTripMicroseconds / kMicrosecondsPerMillisecond,
                       status.leadMicroseconds / static_cast<std::int64_t>(kMicrosecondsPerMillisecond));
}

}

std::string statusLineOf(const ConsoleStatus& status)
{
    using session::ConnectionState;

    if (status.state == ConnectionState::Idle)
    {
        return std::format("{} idle", status.name);
    }

    if (status.state == ConnectionState::Connecting)
    {
        return std::format("{} connecting to the relay", status.name);
    }

    if (status.state == ConnectionState::Joining)
    {
        return std::format("{} joining the match", status.name);
    }

    if (status.state == ConnectionState::Disconnected)
    {
        return std::format("{} disconnected", status.name);
    }

    return inMatch(status, status.state == ConnectionState::Stalled ? "stalled" : "playing");
}

}
