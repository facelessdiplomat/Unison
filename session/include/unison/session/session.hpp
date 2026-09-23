#pragma once

#include <unison/session/input_buffer.hpp>
#include <unison/session/local_input.hpp>
#include <unison/session/repeat_last_input_predictor.hpp>
#include <unison/session/session_config.hpp>
#include <unison/session/snapshot_ring.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace unison::session
{

/// One client's side of a match, played ahead of the relay: every tick simulates the next frame on the
/// local player's input and a guess for everyone else and keeps a snapshot of it to roll back to. The
/// frame and the pipeline belong to the game, which keeps them alive for as long as the session runs.
class Session
{
public:
    Session(sim::Frame& frame, const sim::SystemPipeline& pipeline, const SessionConfig& config, std::size_t localSlot);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;

    /// Replaces the input the local player plays from the next tick on.
    void setLocalInput(std::span<const std::byte> input);

    /// Simulates the frame after the predicted one and keeps its snapshot.
    void tick();

    /// Takes the inputs the relay settled for a frame. Returns false for a frame outside the window: one
    /// already verified, or one too far ahead to hold yet.
    [[nodiscard]] bool confirm(std::uint32_t frameNumber, const sim::FrameInputs& confirmed);

    [[nodiscard]] std::uint32_t predictedFrame() const;

    [[nodiscard]] std::uint32_t verifiedFrame() const;

    [[nodiscard]] const InputBuffer& inputs() const;

    [[nodiscard]] const SnapshotRing& snapshots() const;

private:
    void prepareInputs(std::uint32_t frameNumber);

    void advanceVerified();

    [[nodiscard]] bool isConfirmed(std::uint32_t frameNumber) const;

    sim::Frame& liveFrame;
    const sim::SystemPipeline& systemPipeline;
    SessionConfig config;
    std::size_t localSlot;
    InputBuffer inputBuffer;
    SnapshotRing snapshotRing;
    LocalInput localInput;
    RepeatLastInputPredictor predictor;
    std::uint32_t verified;
    std::uint32_t predicted;
};

}
