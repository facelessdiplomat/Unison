#include <unison/session/session.hpp>

#include <unison/core/contract.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/frame_checksum.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <algorithm>

namespace unison::session
{

namespace
{

std::uint32_t predictionWindowFor(const net::SessionConfig& config)
{
    return config.maxPrediction + 2;
}

}

Session::Session(sim::Frame& frame,
                 const sim::SystemPipeline& pipeline,
                 const net::SessionConfig& config,
                 std::size_t localSlot,
                 std::uint32_t inputDelayFrames,
                 IVerifiedFrameReceiver* verifiedFrames)
    : liveFrame{frame}, systemPipeline{pipeline}, config{config}, inputDelay{inputDelayFrames},
      verifiedFrameReceiver{verifiedFrames},
      inputTimeline{config.slotCount, localSlot, predictionWindowFor(config) + inputDelayFrames + kConfirmationsAhead},
      snapshotRing{predictionWindowFor(config)}, eventHistory{predictionWindowFor(config)}, verified{frame.frameNumber},
      predicted{frame.frameNumber}
{
    UNISON_VERIFY(config.checksumInterval > 0);

    pendingChecksums.reserve(predictionWindowFor(config));
    inputTimeline.evictBelow(verified);
    snapshotRing.store(liveFrame);

    for (std::uint32_t ahead = 1; ahead <= inputDelay; ++ahead)
    {
        inputTimeline.sampleLocal(verified + ahead);
    }
}

void Session::setLocalInput(std::span<const std::byte> input)
{
    inputTimeline.setLocalInput(input);
}

void Session::tick()
{
    rollBack();

    const std::uint32_t next = predicted + 1;

    stalled = !mayPlay(next);

    if (stalled)
    {
        ++stats.stalledTicks;

        return;
    }

    inputTimeline.sampleLocal(next + inputDelay);
    inputTimeline.guessUnconfirmed(next);
    play(next);

    predicted = next;
    ++stats.framesPlayed;
    advanceVerified();
}

bool Session::confirm(std::uint32_t frameNumber, const sim::FrameInputs& confirmed)
{
    if (!inputTimeline.holds(frameNumber))
    {
        return false;
    }

    if (frameNumber <= predicted && !inputTimeline.matches(frameNumber, confirmed))
    {
        firstMispredicted = std::min(firstMispredicted.value_or(frameNumber), frameNumber);
    }

    inputTimeline.confirm(frameNumber, confirmed);
    advanceVerified();

    return true;
}

std::uint32_t Session::predictedFrame() const
{
    return predicted;
}

std::uint32_t Session::verifiedFrame() const
{
    return verified;
}

bool Session::isStalled() const
{
    return stalled;
}

const RollbackStats& Session::rollbackStats() const
{
    return stats;
}

std::span<const VerifiedChecksum> Session::verifiedChecksums() const
{
    return pendingChecksums;
}

void Session::clearVerifiedChecksums()
{
    pendingChecksums.clear();
}

const EventChanges& Session::eventChanges() const
{
    return pendingEventChanges;
}

void Session::clearEventChanges()
{
    pendingEventChanges.raised.clear();
    pendingEventChanges.cancelled.clear();
}

const InputBuffer& Session::inputs() const
{
    return inputTimeline.buffer();
}

const SnapshotRing& Session::snapshots() const
{
    return snapshotRing;
}

void Session::rollBack()
{
    if (!firstMispredicted.has_value())
    {
        return;
    }

    const std::uint32_t first = *firstMispredicted;
    const std::uint32_t depth = predicted - first + 1;
    firstMispredicted.reset();

    ++stats.rollbacks;
    stats.deepestRollback = std::max(stats.deepestRollback, depth);
    stats.resimulatedFrames += depth;

    sim::restoreSnapshot(snapshotRing.snapshotAt(first - 1), liveFrame);

    for (std::uint32_t frameNumber = first; frameNumber <= predicted; ++frameNumber)
    {
        inputTimeline.guessUnconfirmed(frameNumber);
        play(frameNumber);
    }

    advanceVerified();
}

void Session::play(std::uint32_t frameNumber)
{
    sim::advanceFrame(liveFrame, systemPipeline, inputTimeline.inputsAt(frameNumber));
    snapshotRing.store(liveFrame);
    eventHistory.record(frameNumber, liveFrame.events, pendingEventChanges);
}

void Session::advanceVerified()
{
    const std::uint32_t settled = firstMispredicted.has_value() ? *firstMispredicted - 1 : predicted;

    while (verified < settled && inputTimeline.isConfirmed(verified + 1))
    {
        ++verified;
        eventHistory.release(verified, pendingEventChanges);

        const std::optional<std::uint64_t> checksum =
            verified % config.checksumInterval == 0 ? std::optional{sim::checksumOf(snapshotRing.snapshotAt(verified))}
                                                    : std::nullopt;

        if (checksum.has_value())
        {
            pendingChecksums.push_back(VerifiedChecksum{verified, *checksum});
        }

        if (verifiedFrameReceiver != nullptr)
        {
            verifiedFrameReceiver->frameVerified(verified, inputTimeline.inputsAt(verified), checksum);
        }
    }

    inputTimeline.evictBelow(verified);
    snapshotRing.evictBelow(verified);
}

bool Session::mayPlay(std::uint32_t frameNumber) const
{
    const bool staysInsideWindow = frameNumber - verified <= config.maxPrediction;
    const bool isVerifiedAtOnce = verified == predicted && inputTimeline.isConfirmed(frameNumber);

    return staysInsideWindow || isVerifiedAtOnce;
}

}
