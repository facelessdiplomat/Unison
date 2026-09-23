#include <unison/session/session.hpp>

#include <unison/core/contract.hpp>
#include <unison/sim/advance_frame.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <algorithm>

namespace unison::session
{

namespace
{

std::uint32_t windowFor(const SessionConfig& config)
{
    return config.maxPrediction + 2;
}

}

Session::Session(sim::Frame& frame,
                 const sim::SystemPipeline& pipeline,
                 const SessionConfig& config,
                 std::size_t localSlot)
    : liveFrame{frame}, systemPipeline{pipeline}, config{config}, localSlot{localSlot}, inputBuffer{windowFor(config)},
      snapshotRing{windowFor(config)}, localInput{localSlot}, verified{frame.frameNumber}, predicted{frame.frameNumber}
{
    UNISON_VERIFY(config.slotCount <= sim::kMaxSlots);
    UNISON_VERIFY(localSlot < config.slotCount);

    inputBuffer.evictBelow(verified);
    snapshotRing.store(liveFrame);
}

void Session::setLocalInput(std::span<const std::byte> input)
{
    localInput.set(input);
}

void Session::tick()
{
    rollBack();

    const std::uint32_t next = predicted + 1;

    sampleLocalInput(next);
    guessUnconfirmedInputs(next);
    play(next);

    predicted = next;
    advanceVerified();
}

bool Session::confirm(std::uint32_t frameNumber, const sim::FrameInputs& confirmed)
{
    if (!inputBuffer.holds(frameNumber))
    {
        return false;
    }

    if (frameNumber <= predicted && !wasPlayedAs(frameNumber, confirmed))
    {
        firstMispredicted = std::min(firstMispredicted.value_or(frameNumber), frameNumber);
    }

    for (std::size_t slot = 0; slot < config.slotCount; ++slot)
    {
        const bool stored = inputBuffer.store(
            frameNumber, slot, confirmed.bytesAt(slot), confirmed.flagsAt(slot), InputState::Confirmed);
        UNISON_VERIFY(stored);
    }

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

const InputBuffer& Session::inputs() const
{
    return inputBuffer;
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
    firstMispredicted.reset();

    sim::restoreSnapshot(snapshotRing.snapshotAt(first - 1), liveFrame);

    for (std::uint32_t frameNumber = first; frameNumber <= predicted; ++frameNumber)
    {
        guessUnconfirmedInputs(frameNumber);
        play(frameNumber);
    }

    advanceVerified();
}

void Session::play(std::uint32_t frameNumber)
{
    sim::advanceFrame(liveFrame, systemPipeline, inputBuffer.inputsAt(frameNumber));
    snapshotRing.store(liveFrame);
}

void Session::sampleLocalInput(std::uint32_t frameNumber)
{
    if (inputBuffer.stateAt(frameNumber, localSlot) == InputState::Confirmed)
    {
        return;
    }

    const bool sampled = localInput.sampleInto(inputBuffer, frameNumber);
    UNISON_VERIFY(sampled);
}

void Session::guessUnconfirmedInputs(std::uint32_t frameNumber)
{
    for (std::size_t slot = 0; slot < config.slotCount; ++slot)
    {
        if (slot == localSlot || inputBuffer.stateAt(frameNumber, slot) == InputState::Confirmed)
        {
            continue;
        }

        const bool guessed = predictor.predict(inputBuffer, frameNumber, slot);
        UNISON_VERIFY(guessed);
    }
}

void Session::advanceVerified()
{
    const std::uint32_t settled = firstMispredicted.has_value() ? *firstMispredicted - 1 : predicted;

    while (verified < settled && isConfirmed(verified + 1))
    {
        ++verified;
    }

    inputBuffer.evictBelow(verified);
    snapshotRing.evictBelow(verified);
}

bool Session::wasPlayedAs(std::uint32_t frameNumber, const sim::FrameInputs& confirmed) const
{
    const sim::FrameInputs& played = inputBuffer.inputsAt(frameNumber);

    for (std::size_t slot = 0; slot < config.slotCount; ++slot)
    {
        if (played.flagsAt(slot) != confirmed.flagsAt(slot) ||
            !std::ranges::equal(played.bytesAt(slot), confirmed.bytesAt(slot)))
        {
            return false;
        }
    }

    return true;
}

bool Session::isConfirmed(std::uint32_t frameNumber) const
{
    for (std::size_t slot = 0; slot < config.slotCount; ++slot)
    {
        if (inputBuffer.stateAt(frameNumber, slot) != InputState::Confirmed)
        {
            return false;
        }
    }

    return true;
}

}
