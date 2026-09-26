#include <unison/runner/rollback_summary.hpp>

#include <unison/net/protocol.hpp>

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
    return std::format("  {:>9}  {:>9}  {:>10.2f}  {:>10.2f}  {:>7}  {:>13}\n",
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

    std::string summary = std::format("unison_runner: rollbacks by slot\n"
                                      "  {:>9}  rollbacks  per second  mean depth  deepest  stalled ticks\n",
                                      "slot");

    for (const ClientOutcome& client : bySlot)
    {
        summary +=
            rowOf(client.slot == net::kNoSlot ? "spectator" : std::to_string(client.slot), client.rollbacks, tickRate);
    }

    summary += rowOf("all", rollbacksOfAll(clients), tickRate);

    return summary;
}

}
