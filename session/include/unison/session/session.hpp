#pragma once

#include <unison/session/input_buffer.hpp>
#include <unison/session/local_input.hpp>
#include <unison/session/repeat_last_input_predictor.hpp>
#include <unison/session/rollback_stats.hpp>
#include <unison/session/session_config.hpp>
#include <unison/session/snapshot_ring.hpp>
#include <unison/session/verified_checksum.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

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

    /// Plays again every frame since the first one guessed wrong, then simulates the frame after the
    /// predicted one and keeps its snapshot, unless that frame would take the session further ahead of
    /// the verified frame than the config allows and the relay has not settled it yet.
    void tick();

    /// Takes the inputs the relay settled for a frame and notes whether a frame already played was guessed
    /// wrong. Returns false for a frame outside the window: already verified, or too far ahead to hold.
    [[nodiscard]] bool confirm(std::uint32_t frameNumber, const sim::FrameInputs& confirmed);

    [[nodiscard]] std::uint32_t predictedFrame() const;

    [[nodiscard]] std::uint32_t verifiedFrame() const;

    /// Whether the last tick had to wait for the relay instead of simulating a frame.
    [[nodiscard]] bool isStalled() const;

    [[nodiscard]] const RollbackStats& rollbackStats() const;

    /// The checksums of the frames verified on the config's interval since they were last cleared, oldest
    /// first, each taken from the frame's snapshot. Taking them after every tick misses none.
    [[nodiscard]] std::span<const VerifiedChecksum> verifiedChecksums() const;

    void clearVerifiedChecksums();

    [[nodiscard]] const InputBuffer& inputs() const;

    [[nodiscard]] const SnapshotRing& snapshots() const;

private:
    void rollBack();

    void play(std::uint32_t frameNumber);

    void sampleLocalInput(std::uint32_t frameNumber);

    void guessUnconfirmedInputs(std::uint32_t frameNumber);

    void advanceVerified();

    [[nodiscard]] bool mayPlay(std::uint32_t frameNumber) const;

    [[nodiscard]] bool wasPlayedAs(std::uint32_t frameNumber, const sim::FrameInputs& confirmed) const;

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
    std::optional<std::uint32_t> firstMispredicted;
    bool stalled = false;
    RollbackStats stats;
    std::vector<VerifiedChecksum> pendingChecksums;
};

}
