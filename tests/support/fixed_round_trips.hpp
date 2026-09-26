#pragma once

#include <unison/net/round_trip_meter.hpp>
#include <unison/net/transport.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace unison::test
{

/// A round-trip meter that measures what a test tells it to and nothing else.
class FixedRoundTrips final : public net::IRoundTripMeter
{
public:
    void set(net::PeerId peer, std::uint64_t microseconds)
    {
        measured.emplace_back(peer, microseconds);
    }

    [[nodiscard]] std::optional<std::uint64_t> roundTripMicroseconds(net::PeerId peer) const override
    {
        const auto found = std::ranges::find(measured, peer, &std::pair<net::PeerId, std::uint64_t>::first);

        return found == measured.end() ? std::nullopt : std::optional{found->second};
    }

private:
    std::vector<std::pair<net::PeerId, std::uint64_t>> measured;
};

}
