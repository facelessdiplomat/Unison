#pragma once

#include <unison/net/checksum_referee.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/confirmed_log.hpp>
#include <unison/net/input_collector.hpp>
#include <unison/net/late_joins.hpp>
#include <unison/net/match_clock.hpp>
#include <unison/net/outbox.hpp>
#include <unison/net/protocol.hpp>
#include <unison/net/roster.hpp>
#include <unison/net/round_trip_meter.hpp>
#include <unison/net/session_config.hpp>
#include <unison/net/transport.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace unison::net
{

/// How a relay treats players and a lossy network: how long it waits for a missing input once the first
/// input for a frame has arrived, and every how many confirmed frames it sends them all again reliably.
struct RelaySettings
{
    std::uint64_t inputDeadlineMicroseconds = 100'000;
    std::uint32_t reliableResendInterval = 10;
};

/// How many of the newest confirmed frames every confirmation carries when a datagram holds them, so that a
/// confirmation lost on the way costs nothing as long as the next one arrives.
inline constexpr std::uint32_t kRedundantConfirmations = 4;

/// The relay of one match. It lets clients in, seats players in the slots of the config it was given and turns away a
/// client that speaks another protocol, would play another config or finds every slot taken; a member's second hello
/// changes nothing. It confirms a frame once every player has sent an input for it, or at the deadline without the
/// missing ones, sends the confirmation with the frames confirmed just before it to everyone, answers a ping with the
/// frame its clock has due, and tells everyone which players' checksums part ways with the rest. A player joining a
/// running match holds its slot out of play while the player in play with the lowest round trip the meter, when given,
/// measures sends it a snapshot, and plays from the first frame it sends an input for. It never simulates, and it
/// answers through its transport.
class RelayCore final : public IMessageReceiver
{
public:
    RelayCore(ITransport& transport,
              const IClock& clock,
              const SessionConfig& config,
              const RelaySettings& settings = RelaySettings{},
              const IRoundTripMeter* roundTrips = nullptr);

    void receive(PeerId from, Channel channel, std::span<const std::byte> message) override;

    /// Takes a peer that has gone out of the match: its slot is free, and the frames it no longer sends
    /// inputs for are confirmed with the slot absent rather than waited for.
    void peerLeft(PeerId peer) override;

    /// Confirms every frame whose deadline has passed; call it between messages as time goes by.
    void update();

    /// Whether everyone who came into the match has left it.
    [[nodiscard]] bool isEmpty() const;

private:
    void handle(PeerId from, const Hello& hello);

    void handle(PeerId from, const Input& input);

    void handle(PeerId from, const Checksum& checksum);

    void handle(PeerId from, const Ping& ping);

    void handle(PeerId from, const SnapshotChunk& chunk);

    template <typename T>
    void handle(PeerId, const T&)
    {
    }

    void admit(PeerId peer, std::uint8_t slot);

    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] std::optional<PeerId> nearestPlayerInPlay() const;

    void turnAway(PeerId peer, LeaveReason reason);

    void confirmReadyFrames();

    void resendReliably(std::uint32_t lastFrame);

    void sendConfirmed(Channel channel, std::uint32_t firstFrame, std::uint32_t lastFrame);

    void sendToAll(Channel channel, const Message& message);

    const IClock& clock;
    SessionConfig config;
    RelaySettings settings;
    const IRoundTripMeter* roundTrips;
    std::uint64_t configHash;
    Roster roster;
    Outbox outbox;
    InputCollector inputs;
    MatchClock matchClock;
    ChecksumReferee referee;
    ConfirmedLog confirmedLog;
    LateJoins lateJoins;
    std::vector<std::byte> confirmedSlots;
    std::uint32_t framesPerDatagram;
};

}
