#include <unison/runner/rollback_summary.hpp>

#include <algorithm>
#include <format>
#include <string_view>
#include <vector>

namespace unison::runner
{

namespace
{

std::string rowOf(std::string_view label, const session::RollbackStats& rollbacks, std::uint16_t tickRate)
{
    return std::format("  {:>4}  {:>9}  {:>10.2f}  {:>10.2f}  {:>7}  {:>13}\n",
                       label,
                       rollbacks.rollbacks,
                       rollbacks.rollbacksPerSecond(tickRate),
                       rollbacks.meanRollbackDepth(),
                       rollbacks.deepestRollback,
                       rollbacks.stalledTicks);
}

}

std::string rollbackSummaryOf(std::span<const ClientOutcome> clients, std::uint16_t tickRate)
{
    std::vector<ClientOutcome> bySlot{clients.begin(), clients.end()};
    std::ranges::sort(bySlot, {}, &ClientOutcome::slot);

    std::string summary = "unison_runner: rollbacks by slot\n"
                          "  slot  rollbacks  per second  mean depth  deepest  stalled ticks\n";

    for (const ClientOutcome& client : bySlot)
    {
        summary += rowOf(std::to_string(client.slot), client.rollbacks, tickRate);
    }

    summary += rowOf("all", rollbacksOfAll(clients), tickRate);

    return summary;
}

}
