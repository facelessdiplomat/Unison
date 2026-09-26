#pragma once

#include <unison/net/protocol.hpp>
#include <unison/net/transport.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace unison::net
{

/// The clients a relay has let into its match and the slot each of them plays. It decides nothing about
/// who may come in, only records who has: a player holds a slot of the match nobody else holds, a spectator
/// holds none, a player still joining a running match holds a slot it does not play yet, and a player whose peer
/// has gone keeps its slot held for a while.
class Roster
{
public:
    /// One client let in, the slot it plays, `kNoSlot` for a spectator, the first frame it plays it in,
    /// `kNotPlayingYet` while it is still joining, the token that wins the slot back after a drop, nought for none, and
    /// the moment in microseconds its slot is held until once its peer has gone.
    struct Member
    {
        PeerId peer{};
        std::uint8_t slot = kNoSlot;
        std::uint32_t playsFrom = 0;
        std::uint64_t reconnectToken = 0;
        std::optional<std::uint64_t> heldUntil;
    };

    /// The first frame of a member that plays no frame yet, later than any frame a match reaches.
    static constexpr std::uint32_t kNotPlayingYet = 0xFFFFFFFFU;

    /// A match of more slots than a mask of slots has bits breaks a contract.
    explicit Roster(std::uint8_t slotCount);

    /// Lets a client in with the slot it plays and its reconnect token; a slot the match does not have or someone
    /// already holds breaks a contract.
    void admit(PeerId peer, std::uint8_t slot, std::uint64_t reconnectToken = 0);

    /// Lets a player in who holds its slot but plays no frame of it yet, as one joining a running match; a slot the
    /// match does not have or someone already holds breaks a contract.
    void admitJoining(PeerId peer, std::uint8_t slot, std::uint64_t reconnectToken = 0);

    /// Puts the slot of a member still joining in play from a frame on; any other member breaks a contract.
    void startPlaying(PeerId peer, std::uint32_t fromFrame);

    /// Takes a member out, freeing the slot it held; a peer that is no member changes nothing.
    void remove(PeerId peer);

    /// Keeps the slot of a member whose peer has gone held until a moment, in play but no longer awaited; a peer that
    /// is no member changes nothing.
    void holdSlotOf(PeerId peer, std::uint64_t until);

    /// Takes out every member whose slot was held until a moment no later than `now`.
    void releaseHeldSlots(std::uint64_t now);

    /// The lowest slot nobody holds, or `kNoSlot` when every one of them is taken.
    [[nodiscard]] std::uint8_t freeSlot() const;

    [[nodiscard]] bool isMember(PeerId peer) const;

    /// Whether a member holds a slot it plays no frame of yet.
    [[nodiscard]] bool isJoining(PeerId peer) const;

    [[nodiscard]] bool isEmpty() const;

    /// The slot a member plays: `kNoSlot` for a spectator and for a peer that is not in the match.
    [[nodiscard]] std::uint8_t slotOf(PeerId peer) const;

    /// One bit for every slot a player holds and plays at the frame.
    [[nodiscard]] std::uint8_t slotsInPlayAt(std::uint32_t frame) const;

    /// One bit for every slot in play at the frame whose player's peer has not gone: the slots the relay waits for.
    [[nodiscard]] std::uint8_t slotsAwaitedAt(std::uint32_t frame) const;

    /// Everyone let in, in the order they came.
    [[nodiscard]] std::span<const Member> members() const;

private:
    [[nodiscard]] bool isHeld(std::uint8_t slot) const;

    std::uint8_t slotCount;
    std::vector<Member> admitted;
};

}
