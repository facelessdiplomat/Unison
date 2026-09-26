#include <unison/console/status_line.hpp>

#include <bit>
#include <format>
#include <limits>

namespace unison::console
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerMillisecond = 1'000;
constexpr int kSlotBits = std::numeric_limits<std::uint8_t>::digits;

std::string seatOf(const ConsoleStatus& status)
{
    if (status.slot == net::kNoSlot)
    {
        return "watching";
    }

    return std::format(
        "{} in slot {}", status.state == session::ConnectionState::Stalled ? "stalled" : "playing", status.slot);
}

std::string inMatch(const ConsoleStatus& status)
{
    return std::format("{} {}, verified {}, predicted {}, rollbacks in the last second {}, "
                       "round trip {} ms, lead {} ms",
                       status.name,
                       seatOf(status),
                       status.verifiedFrame,
                       status.predictedFrame,
                       status.rollbacksLastSecond,
                       status.roundTripMicroseconds / kMicrosecondsPerMillisecond,
                       status.leadMicroseconds / static_cast<std::int64_t>(kMicrosecondsPerMillisecond));
}

std::string standingOf(const ConsoleStatus& status)
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

    return inMatch(status);
}

std::string slotsOf(std::uint8_t slotBits)
{
    std::string slots;

    for (int slot = 0; slot < kSlotBits; ++slot)
    {
        if ((slotBits & (1U << slot)) != 0U)
        {
            slots += slots.empty() ? std::format("{}", slot) : std::format(", {}", slot);
        }
    }

    return slots;
}

std::string desyncOf(const std::optional<net::Desync>& desync)
{
    if (!desync.has_value())
    {
        return std::string{};
    }

    return std::format(", desync on frame {} by {} {}",
                       desync->frame,
                       std::has_single_bit(desync->minoritySlots) ? "slot" : "slots",
                       slotsOf(desync->minoritySlots));
}

}

std::string statusLineOf(const ConsoleStatus& status)
{
    return standingOf(status) + desyncOf(status.desync);
}

}
