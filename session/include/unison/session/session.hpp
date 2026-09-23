#pragma once

#include <unison/net/session_config.hpp>
#include <unison/session/event_history.hpp>
#include <unison/session/input_buffer.hpp>
#include <unison/session/input_timeline.hpp>
#include <unison/session/rollback_stats.hpp>
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

/// How many frames past its prediction window a session holds the relay's confirmations for, about two seconds
/// at 60 Hz: a client that stalled through an outage keeps every confirmation that arrives meanwhile, since the
/// relay sends each frame reliably only once, and plays through them to catch up.
inline constexpr std::uint32_t kConfirmationsAhead = 128;

/// One client's side of a match, played ahead of the relay: every tick simulates the next frame on the
/// local player's input and a guess for everyone else and keeps a snapshot of it to roll back to. The
/// frame and the pipeline belong to the game, which keeps them alive for as long as the session runs.
class Session
{
public:
    /// An input delay makes every tick play the local input it samples that many frames later, trading
    /// responsiveness for fewer rollbacks; until then the local player plays the neutral input.
    Session(sim::Frame& frame,
            const sim::SystemPipeline& pipeline,
            const net::SessionConfig& config,
            std::size_t localSlot,
            std::uint32_t inputDelayFrames = 0);

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

    /// The predicted events raised and taken back by the frames played since the changes were last
    /// cleared: every predicted event of a frame played the first time, and what a replay changed.
    [[nodiscard]] const EventChanges& eventChanges() const;

    void clearEventChanges();

    [[nodiscard]] const InputBuffer& inputs() const;

    [[nodiscard]] const SnapshotRing& snapshots() const;

private:
    void rollBack();

    void play(std::uint32_t frameNumber);

    void advanceVerified();

    [[nodiscard]] bool mayPlay(std::uint32_t frameNumber) const;

    sim::Frame& liveFrame;
    const sim::SystemPipeline& systemPipeline;
    net::SessionConfig config;
    std::uint32_t inputDelay;
    InputTimeline inputTimeline;
    SnapshotRing snapshotRing;
    EventHistory eventHistory;
    std::uint32_t verified;
    std::uint32_t predicted;
    std::optional<std::uint32_t> firstMispredicted;
    bool stalled = false;
    RollbackStats stats;
    std::vector<VerifiedChecksum> pendingChecksums;
    EventChanges pendingEventChanges;
};

}
