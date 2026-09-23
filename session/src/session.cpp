#include <unison/session/session.hpp>

#include <unison/core/contract.hpp>
#include <unison/sim/advance_frame.hpp>

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
    const std::uint32_t next = predicted + 1;

    prepareInputs(next);
    sim::advanceFrame(liveFrame, systemPipeline, inputBuffer.inputsAt(next));
    snapshotRing.store(liveFrame);

    predicted = next;
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

void Session::prepareInputs(std::uint32_t frameNumber)
{
    const bool sampled = localInput.sampleInto(inputBuffer, frameNumber);
    UNISON_VERIFY(sampled);

    for (std::size_t slot = 0; slot < config.slotCount; ++slot)
    {
        if (slot != localSlot)
        {
            const bool guessed = predictor.predict(inputBuffer, frameNumber, slot);
            UNISON_VERIFY(guessed);
        }
    }
}

}
