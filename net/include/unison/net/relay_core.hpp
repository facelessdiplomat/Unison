#pragma once

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

/// The relay of one match. It lets clients in, seats players in the slots of the config it was given and
/// turns away a client that speaks another protocol, would play another config or finds every slot taken.
/// It confirms a frame once every player has sent an input for it and sends the confirmation to everyone.
/// It never simulates, and it answers through the transport it was given.
class RelayCore final : public IMessageReceiver
{
public:
    RelayCore(ITransport& transport, const SessionConfig& config);

    void receive(PeerId from, Channel channel, std::span<const std::byte> message) override;

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
    SessionConfig config;
    std::uint64_t configHash;
    std::vector<Member> members;
    InputCollector inputs;
    std::vector<std::byte> confirmedSlots;
    std::array<std::byte, kMaxDatagramSize> sendBuffer{};
};

}
