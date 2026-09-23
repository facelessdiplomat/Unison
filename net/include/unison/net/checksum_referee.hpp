#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace unison::net
{

/// Compares the checksums the players report for each frame and finds who parts ways with the rest. The
/// majority is a checksum more than half of them report; without one, every player is in the minority. At
/// most 64 frames wait for their reports, the oldest giving way to a newer one.
class ChecksumReferee
{
public:
    /// Records a player's checksum for a frame. Once every slot in play, one bit per slot, has reported the
    /// frame, returns the mask of slots in the minority, empty when all agree; until then returns nothing.
    /// A second report from a slot for a frame is ignored.
    [[nodiscard]] std::optional<std::uint8_t>
    record(std::uint32_t frame, std::uint8_t slot, std::uint64_t checksum, std::uint8_t slotsInPlay);

private:
    static constexpr std::size_t kSlots = 8;

    struct Reports
    {
        std::uint32_t frame = 0;
        std::uint8_t reported = 0;
        std::array<std::uint64_t, kSlots> checksums{};
    };

    [[nodiscard]] Reports& reportsFor(std::uint32_t frame);

    [[nodiscard]] static std::uint8_t minorityOf(const Reports& reports);

    [[nodiscard]] static std::uint8_t slotsReporting(const Reports& reports, std::uint64_t checksum);

    std::vector<Reports> pending;
};

}
