#include <unison/runner/checksum_ledger.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>

namespace unison::runner
{

ChecksumLedger::ChecksumLedger(std::size_t clients) : reports(clients)
{
}

void ChecksumLedger::record(std::size_t client, std::uint32_t frame, std::uint64_t checksum)
{
    UNISON_VERIFY(client < reports.size());

    if (client >= reports.size())
    {
        return;
    }

    reports[client].insert_or_assign(frame, checksum);
}

std::optional<Disagreement> ChecksumLedger::firstDisagreement() const
{
    if (reports.empty())
    {
        return std::nullopt;
    }

    for (const auto& [frame, checksum] : reports.front())
    {
        if (!isReportedByAll(frame))
        {
            continue;
        }

        const bool agree = std::ranges::all_of(
            reports, [frame, checksum](const auto& byFrame) { return byFrame.find(frame)->second == checksum; });

        if (agree)
        {
            continue;
        }

        Disagreement disagreement{frame, {}};

        for (const auto& byFrame : reports)
        {
            disagreement.checksums.push_back(byFrame.find(frame)->second);
        }

        return disagreement;
    }

    return std::nullopt;
}

std::size_t ChecksumLedger::framesReportedByAll() const
{
    if (reports.empty())
    {
        return 0;
    }

    return static_cast<std::size_t>(
        std::ranges::count_if(reports.front(), [this](const auto& report) { return isReportedByAll(report.first); }));
}

bool ChecksumLedger::isReportedByAll(std::uint32_t frame) const
{
    return std::ranges::all_of(reports, [frame](const auto& byFrame) { return byFrame.contains(frame); });
}

}
