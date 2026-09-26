#pragma once

#include <unison/net/transport.hpp>

#include <cstdint>
#include <optional>

namespace unison::net
{

/// Tells how long a round trip to a peer takes, as a transport measures it; nothing for a peer it has no measure of.
class IRoundTripMeter
{
public:
    IRoundTripMeter() = default;
    virtual ~IRoundTripMeter() = default;

    IRoundTripMeter(const IRoundTripMeter&) = delete;
    IRoundTripMeter& operator=(const IRoundTripMeter&) = delete;
    IRoundTripMeter(IRoundTripMeter&&) = delete;
    IRoundTripMeter& operator=(IRoundTripMeter&&) = delete;

    [[nodiscard]] virtual std::optional<std::uint64_t> roundTripMicroseconds(PeerId peer) const = 0;
};

}
