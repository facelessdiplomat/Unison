#include <unison/net/donor_choice.hpp>

#include <cstdint>
#include <limits>

namespace unison::net
{

std::optional<PeerId> nearestAwaitedPlayer(const Roster& roster, const IRoundTripMeter* roundTrips)
{
    std::optional<Roster::Member> nearest;
    std::uint64_t nearestRoundTrip = std::numeric_limits<std::uint64_t>::max();

    for (const Roster::Member& member : roster.members())
    {
        const bool isAwaited =
            member.slot != kNoSlot && member.awaitedFrom != Roster::kNotPlayingYet && !member.heldUntil.has_value();

        if (!isAwaited)
        {
            continue;
        }

        const std::optional<std::uint64_t> measured =
            roundTrips == nullptr ? std::nullopt : roundTrips->roundTripMicroseconds(member.peer);
        const std::uint64_t roundTrip = measured.value_or(std::numeric_limits<std::uint64_t>::max());
        const bool isNearer = !nearest.has_value() || roundTrip < nearestRoundTrip ||
                              (roundTrip == nearestRoundTrip && member.slot < nearest->slot);

        if (isNearer)
        {
            nearest = member;
            nearestRoundTrip = roundTrip;
        }
    }

    return nearest.has_value() ? std::optional{nearest->peer} : std::nullopt;
}

}
