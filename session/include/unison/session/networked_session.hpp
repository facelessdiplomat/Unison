#pragma once

#include <unison/core/error.hpp>
#include <unison/net/outbox.hpp>
#include <unison/net/protocol.hpp>
#include <unison/net/session_config.hpp>
#include <unison/net/transport.hpp>
#include <unison/session/awaited_snapshot.hpp>
#include <unison/session/catch_up.hpp>
#include <unison/session/connection_states.hpp>
#include <unison/session/newest_inputs.hpp>
#include <unison/session/session.hpp>
#include <unison/session/snapshot_donor.hpp>
#include <unison/session/time_sync.hpp>
#include <unison/session/verified_frame_fan_out.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <tl/expected.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace unison::session
{

/// How often a playing client pings the relay to time the round trip and to see how far ahead it runs.
inline constexpr std::uint64_t kPingIntervalMicroseconds = 100'000;

/// One client's side of a match played against a relay: it asks to join, plays a session in the slot the
/// relay gives it, sends its own inputs with the ones before them, settles every frame the relay confirms,
/// reports the checksums of the frames it verifies and pings the relay to keep pace with it. It listens to
/// the relay and to nobody else.
class NetworkedSession final : public net::IMessageReceiver
{
public:
    /// A receiver, when given, hears of every frame the session verifies once the relay has let the client in,
    /// and must outlive the client.
    NetworkedSession(sim::Frame& frame,
                     const sim::SystemPipeline& pipeline,
                     const net::SessionConfig& config,
                     net::ITransport& transport,
                     net::PeerId relay,
                     std::uint32_t inputDelayFrames = 0,
                     IVerifiedFrameReceiver* receiver = nullptr);

    /// Asks the relay to let this client play the config it was made with; given the token of an earlier welcome, asks
    /// for the slot that welcome gave back after a drop.
    void join(std::uint64_t reconnectToken = 0);

    /// Replaces the input the local player plays from the next tick on, even before the relay's welcome. More
    /// bytes than a slot holds break a contract.
    void setLocalInput(std::span<const std::byte> input);

    /// Takes in what the relay sent by `now`, in microseconds, and pings the relay when a ping is due.
    void update(std::uint64_t now);

    /// While playing, ticks the session and sends the relay the newest inputs and the checksums of the frames
    /// verified since the last tick.
    void tick();

    /// The ticks the host adds to the frame it plays after the last update to keep to the relay's clock: one
    /// fewer, one more, or none; a late joiner still catching up adds up to `kCatchUpExtraTicks`.
    [[nodiscard]] std::int32_t takeTickCorrection();

    void receive(net::PeerId from, net::Channel channel, std::span<const std::byte> message) override;

    /// The relay reached: a connecting client is joining from then on.
    void peerArrived(net::PeerId peer) override;

    /// The relay gone: the client is disconnected from then on.
    void peerLeft(net::PeerId peer) override;

    [[nodiscard]] ConnectionState state() const;

    /// Every state the client has moved into since the host last cleared them, oldest first.
    [[nodiscard]] std::span<const ConnectionState> connectionChanges() const;

    /// Clears the connection changes once the host has taken them.
    void clearConnectionChanges();

    /// The slot the relay gave this client, `kNoSlot` before it has given one.
    [[nodiscard]] std::uint8_t localSlot() const;

    /// The frame the relay let this client in at: nought for a client that joined a match at its start, the frame of
    /// the snapshot it restores for one that joined late.
    [[nodiscard]] std::uint32_t startFrame() const;

    /// The token the relay's welcome carried, which wins the slot back after a drop: nought before a welcome and for a
    /// spectator.
    [[nodiscard]] std::uint64_t reconnectToken() const;

    /// The session the client plays, or nothing before the relay has let the client in.
    [[nodiscard]] const Session* session() const;

    /// Clears the event changes of the session once the host has taken them.
    void clearEventChanges();

    [[nodiscard]] const TimeSync& timeSync() const;

    /// The last desync the relay reported, if it reported one.
    [[nodiscard]] std::optional<net::Desync> lastDesync() const;

private:
    void handle(const net::Welcome& welcome);

    void handle(const net::Confirmed& confirmed);

    void handle(const net::Pong& pong);

    void handle(const net::Kick& kick);

    void handle(const net::Desync& desync);

    void handle(const net::SnapshotRequest& request);

    void handle(const net::SnapshotChunk& chunk);

    void startFrom(const tl::expected<sim::FrameSnapshot, Error>& snapshot);

    template <typename T>
    void handle(const T&)
    {
    }

    void sendInputs();

    void sendChecksums();

    sim::Frame& frame;
    const sim::SystemPipeline& pipeline;
    net::SessionConfig config;
    net::ITransport& transport;
    net::PeerId relay;
    std::uint32_t inputDelay;
    net::Outbox outbox;
    SnapshotDonor donor;
    VerifiedFrameFanOut verifiedFrames;
    TimeSync pace;
    ConnectionStates connection;
    std::optional<net::Welcome> welcomed;
    std::optional<Session> played;
    std::optional<AwaitedSnapshot> awaitedSnapshot;
    CatchUp catchUp;
    std::optional<net::Desync> reportedDesync;
    std::uint64_t updatedAt = 0;
    std::uint64_t nextPingAt = 0;
    std::array<std::byte, sim::kMaxInputSize> localInput{};
    std::array<std::byte, kRedundantInputs * sim::kMaxInputSize> inputBatch{};
};

}
