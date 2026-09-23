#pragma once

#include <unison/net/checksum_referee.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/confirmed_log.hpp>
#include <unison/net/input_collector.hpp>
#include <unison/net/match_clock.hpp>
#include <unison/net/outbox.hpp>
#include <unison/net/protocol.hpp>
#include <unison/net/roster.hpp>
#include <unison/net/session_config.hpp>
#include <unison/net/transport.hpp>

#include <cstddef>
#include <cstdint>
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

/// The relay of one match. It lets clients in, seats players in the slots of the config it was given and
/// turns away a client that speaks another protocol, would play another config or finds every slot taken.
/// It confirms a frame once every player has sent an input for it, or at the deadline without the missing
/// ones, sends the confirmation with the frames confirmed just before it to everyone, answers a ping with the
/// frame its clock has due, and tells everyone which players' checksums part ways with the rest. It never
/// simulates, and it answers through its transport.
class RelayCore final : public IMessageReceiver
{
public:
    RelayCore(ITransport& transport,
              const IClock& clock,
              const SessionConfig& config,
              const RelaySettings& settings = RelaySettings{});

    void receive(PeerId from, Channel channel, std::span<const std::byte> message) override;

    /// Confirms every frame whose deadline has passed; call it between messages as time goes by.
    void update();

private:
    void handle(PeerId from, const Hello& hello);

    void handle(PeerId from, const Input& input);

    void handle(PeerId from, const Checksum& checksum);

    void handle(PeerId from, const Ping& ping);

    template <typename T>
    void handle(PeerId, const T&)
    {
    }

    void admit(PeerId peer, std::uint8_t slot);

    void turnAway(PeerId peer, LeaveReason reason);

    void confirmReadyFrames();

    void resendReliably(std::uint32_t lastFrame);

    void sendConfirmed(Channel channel, std::uint32_t firstFrame, std::uint32_t lastFrame);

    void sendToAll(Channel channel, const Message& message);

    const IClock& clock;
    SessionConfig config;
    RelaySettings settings;
    std::uint64_t configHash;
    Roster roster;
    Outbox outbox;
    InputCollector inputs;
    MatchClock matchClock;
    ChecksumReferee referee;
    ConfirmedLog confirmedLog;
    std::vector<std::byte> confirmedSlots;
    std::uint32_t framesPerDatagram;
};

}
