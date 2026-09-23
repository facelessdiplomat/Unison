#include <unison/net/checksum_referee.hpp>

#include <algorithm>
#include <bit>

namespace unison::net
{

namespace
{

constexpr std::size_t kMostPendingFrames = 64;

bool isSet(std::uint8_t mask, std::uint8_t slot)
{
    return ((mask >> slot) & 1U) != 0U;
}

}

std::optional<std::uint8_t>
ChecksumReferee::record(std::uint32_t frame, std::uint8_t slot, std::uint64_t checksum, std::uint8_t slotsInPlay)
{
    if (slot >= kSlots || !isSet(slotsInPlay, slot))
    {
        return std::nullopt;
    }

    Reports& reports = reportsFor(frame);

    if (isSet(reports.reported, slot))
    {
        return std::nullopt;
    }

    reports.reported = static_cast<std::uint8_t>(reports.reported | (1U << slot));
    reports.checksums[slot] = checksum;

    if ((reports.reported & slotsInPlay) != slotsInPlay)
    {
        return std::nullopt;
    }

    const std::uint8_t minority = minorityOf(reports);
    std::erase_if(pending, [frame](const Reports& waiting) { return waiting.frame == frame; });

    return minority;
}

ChecksumReferee::Reports& ChecksumReferee::reportsFor(std::uint32_t frame)
{
    const auto found = std::ranges::find(pending, frame, &Reports::frame);

    if (found != pending.end())
    {
        return *found;
    }

    if (pending.size() == kMostPendingFrames)
    {
        pending.erase(std::ranges::min_element(pending, {}, &Reports::frame));
    }

    return pending.emplace_back(Reports{frame, 0, {}});
}

std::uint8_t ChecksumReferee::minorityOf(const Reports& reports)
{
    const int reporting = std::popcount(reports.reported);

    for (std::uint8_t candidate = 0; candidate < kSlots; ++candidate)
    {
        if (!isSet(reports.reported, candidate))
        {
            continue;
        }

        const std::uint8_t agreeing = slotsReporting(reports, reports.checksums[candidate]);

        if (std::popcount(agreeing) * 2 > reporting)
        {
            return static_cast<std::uint8_t>(reports.reported & ~agreeing);
        }
    }

    return reports.reported;
}

std::uint8_t ChecksumReferee::slotsReporting(const Reports& reports, std::uint64_t checksum)
{
    std::uint8_t agreeing = 0;

    for (std::uint8_t slot = 0; slot < kSlots; ++slot)
    {
        if (isSet(reports.reported, slot) && reports.checksums[slot] == checksum)
        {
            agreeing = static_cast<std::uint8_t>(agreeing | (1U << slot));
        }
    }

    return agreeing;
}

}
