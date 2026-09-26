#pragma once

#include <unison/net/roster.hpp>
#include <unison/net/round_trip_meter.hpp>
#include <unison/net/transport.hpp>

#include <optional>

namespace unison::net
{

/// The player of a roster the relay waits for, neither away nor catching up, with the lowest round trip a meter, when
/// given, measures, the lowest slot among equals or where nothing is measured; nothing when no player is waited for.
[[nodiscard]] std::optional<PeerId> nearestAwaitedPlayer(const Roster& roster, const IRoundTripMeter* roundTrips);

}
