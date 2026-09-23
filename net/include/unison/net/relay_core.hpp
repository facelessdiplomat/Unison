#pragma once

#include <unison/net/clock.hpp>
#include <unison/net/input_collector.hpp>
#include <unison/net/protocol.hpp>
#include <unison/net/session_config.hpp>
#include <unison/net/transport.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace unison::net
{

/// How a relay treats a player who falls behind: how long it waits for a missing input once the first input
/// for a frame has arrived before it confirms the frame without it.
struct RelaySettings
{
    std::uint64_t inputDeadlineMicroseconds = 100'000;
};

/// The relay of one match. It lets clients in, seats players in the slots of the config it was given and
/// turns away a client that speaks another protocol, would play another config or finds every slot taken.
/// It confirms a frame once every player has sent an input for it, or at the deadline without the missing
/// ones, and sends the confirmation to everyone. It never simulates, and it answers through its transport.
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
    struct Member
    {
        PeerId peer{};
        std::uint8_t slot = kNoSlot;
    };

    void handle(PeerId from, const Hello& hello);

    void handle(PeerId from, const Input& input);

    template <typename T>
    void handle(PeerId, const T&)
    {
    }

    void admit(PeerId peer, std::uint8_t slot);

    void turnAway(PeerId peer, LeaveReason reason);

    void confirmReadyFrames();

    void sendTo(PeerId peer, Channel channel, const Message& message);

    void sendToAll(Channel channel, const Message& message);

    [[nodiscard]] std::uint8_t freeSlot() const;

    [[nodiscard]] std::uint8_t slotOf(PeerId peer) const;

    [[nodiscard]] std::uint8_t slotsInPlay() const;

    ITransport& transport;
    const IClock& clock;
    SessionConfig config;
    RelaySettings settings;
    std::uint64_t configHash;
    std::vector<Member> members;
    InputCollector inputs;
    std::vector<std::byte> confirmedSlots;
    std::array<std::byte, kMaxDatagramSize> sendBuffer{};
};

}
