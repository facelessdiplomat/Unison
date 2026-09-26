#include <unison/runner/run_outcome.hpp>

#include <unison/runner/rollback_summary.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>
#include <cstddef>
#include <format>

namespace unison::runner
{

namespace
{

constexpr int kVerifiedEveryFrame = 0;
constexpr int kMissedFrames = 1;
constexpr int kPartedWays = 2;
constexpr int kOverflowedTheWindow = 3;

std::uint32_t deepestRollbackOf(const RunOutcome& outcome)
{
    return rollbacksOfAll(outcome.clients).deepestRollback;
}

bool hasOverflowed(const RunOutcome& outcome)
{
    return deepestRollbackOf(outcome) > outcome.predictionWindow;
}

std::string reportOfDisagreement(const RunOutcome& outcome, const Disagreement& disagreement)
{
    UNISON_VERIFY(outcome.clients.size() == disagreement.checksums.size());

    const std::size_t clients = std::min(outcome.clients.size(), disagreement.checksums.size());
    std::string report = std::format("unison_runner: the clients part ways at frame {}\n", disagreement.frame);

    for (std::size_t client = 0; client < clients; ++client)
    {
        report += std::format("  slot {}: {:016x}\n", outcome.clients[client].slot, disagreement.checksums[client]);
    }

    return report;
}

std::string verdictOf(const RunOutcome& outcome, const RunnerOptions& options)
{
    if (outcome.disagreement.has_value())
    {
        return reportOfDisagreement(outcome, *outcome.disagreement);
    }

    if (hasOverflowed(outcome))
    {
        return std::format("unison_runner: a client rolled back {} frames, deeper than its window of {}\n",
                           deepestRollbackOf(outcome),
                           outcome.predictionWindow);
    }

    if (!outcome.isComplete)
    {
        return std::format("unison_runner: the slowest client verified {} of {} frames in {} host frames\n",
                           outcome.fewestVerifiedFrames,
                           options.frames,
                           outcome.hostFrames);
    }

    return std::format("unison_runner: {} players verified {} frames in {} host frames\n"
                       "unison_runner: the clients agree on the checksums of {} frames\n",
                       options.players,
                       options.frames,
                       outcome.hostFrames,
                       outcome.framesCompared);
}

std::string lateJoinsOf(const RunOutcome& outcome)
{
    std::string report;

    for (const ClientOutcome& client : outcome.clients)
    {
        if (client.startFrame > 0)
        {
            report += std::format("unison_runner: slot {} {}, from a snapshot of frame {}\n",
                                  client.slot,
                                  client.hasComeBack ? "came back" : "joined late",
                                  client.startFrame);
        }
    }

    return report;
}

}

int exitCodeOf(const RunOutcome& outcome)
{
    if (outcome.disagreement.has_value())
    {
        return kPartedWays;
    }

    if (hasOverflowed(outcome))
    {
        return kOverflowedTheWindow;
    }

    return outcome.isComplete ? kVerifiedEveryFrame : kMissedFrames;
}

std::string reportOf(const RunOutcome& outcome, const RunnerOptions& options)
{
    return verdictOf(outcome, options) + lateJoinsOf(outcome) + rollbackSummaryOf(outcome.clients, options.tickRate);
}

}
