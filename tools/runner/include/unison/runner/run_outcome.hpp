#pragma once

#include <unison/runner/checksum_ledger.hpp>
#include <unison/runner/runner_options.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace unison::runner
{

/// How a run ended: whether every client verified every frame asked for, in how many host frames, how far the
/// slowest client got, the deepest rollback any client took against the prediction window it was allowed, the
/// slot every client played, how many frames every client reported a checksum for, and where the clients
/// parted ways, if they did.
struct RunOutcome
{
    bool isComplete = false;
    std::uint32_t hostFrames = 0;
    std::uint32_t fewestVerifiedFrames = 0;
    std::uint32_t deepestRollback = 0;
    std::uint32_t predictionWindow = 0;
    std::vector<std::uint8_t> slots;
    std::size_t framesCompared = 0;
    std::optional<Disagreement> disagreement;
};

/// The exit code a run ends with: two when the clients parted ways, three when one of them rolled back deeper
/// than its prediction window, one when not every frame was verified, nought otherwise.
[[nodiscard]] int exitCodeOf(const RunOutcome& outcome);

/// What the runner prints about a run: how it went, and for a desync the frame and every client's slot and
/// checksum of it; a desync without the slot of every checksum breaks a contract.
[[nodiscard]] std::string reportOf(const RunOutcome& outcome, const RunnerOptions& options);

}
