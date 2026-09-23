#pragma once

#include <unison/net/outbox.hpp>
#include <unison/net/protocol.hpp>
#include <unison/net/session_config.hpp>
#include <unison/net/transport.hpp>
#include <unison/session/session.hpp>
#include <unison/session/time_sync.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace unison::session
{

/// Where a client stands with the relay: not asked to join yet, waiting to be let in, playing in the slot
/// it was given, or sent away.
enum class ConnectionState : std::uint8_t
{
    Idle,
    Joining,
    Playing,
    Disconnected
};

/// How many of its newest inputs a client sends in every input message, so that a message lost on the way
/// costs nothing as long as the next one arrives.
inline constexpr std::uint32_t kRedundantInputs = 4;

/// How often a playing client pings the relay to time the round trip and to see how far ahead it runs.
inline constexpr std::uint64_t kPingIntervalMicroseconds = 100'000;

/// One client's side of a match played against a relay: it asks to join, plays a session in the slot the
/// relay gives it, sends its own inputs with the ones before them, settles every frame the relay confirms,
/// reports the checksums of the frames it verifies and pings the relay to keep pace with it. It listens to
/// the relay and to nobody else.
class NetworkedSession final : public net::IMessageReceiver
{
public:
    NetworkedSession(sim::Frame& frame,
                     const sim::SystemPipeline& pipeline,
                     const net::SessionConfig& config,
                     net::ITransport& transport,
                     net::PeerId relay,
                     std::uint32_t inputDelayFrames = 0);

    /// Asks the relay to let this client play the config it was made with.
    void join();

    /// Replaces the input the local player plays from the next tick on, even before the relay's welcome. More
    /// bytes than a slot holds break a contract.
    void setLocalInput(std::span<const std::byte> input);

    /// Takes in what the relay sent by `now`, in microseconds, and pings the relay when a ping is due.
    void update(std::uint64_t now);

    /// While playing, ticks the session and sends the relay the newest inputs and the checksums of the frames
    /// verified since the last tick.
    void tick();

    /// The ticks the host adds to its next frame to keep pace with the relay: one fewer, one more, or none.
    [[nodiscard]] std::int32_t takeTickCorrection();

    void receive(net::PeerId from, net::Channel channel, std::span<const std::byte> message) override;

    [[nodiscard]] ConnectionState state() const;

    /// The slot the relay gave this client, `kNoSlot` before it has given one.
    [[nodiscard]] std::uint8_t localSlot() const;

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
    TimeSync pace;
    ConnectionState connection = ConnectionState::Idle;
    std::uint8_t givenSlot = net::kNoSlot;
    std::optional<Session> played;
    std::optional<net::Desync> reportedDesync;
    std::uint64_t updatedAt = 0;
    std::uint64_t nextPingAt = 0;
    std::array<std::byte, sim::kMaxInputSize> localInput{};
    std::array<std::byte, kRedundantInputs * sim::kMaxInputSize> inputBatch{};
};

}
