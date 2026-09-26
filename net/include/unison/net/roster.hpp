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
/// holds none, a player still joining a running match holds a slot it does not play yet, a player whose peer
/// has gone keeps its slot held for a while, and one back with its token catches up before it is awaited again.
class Roster
{
public:
    /// One client let in, the slot it plays, `kNoSlot` for a spectator, the first frame it plays it in and the first
    /// the relay waits for its input in, both `kNotPlayingYet` while it is still joining and the second while it
    /// catches up, the token that wins the slot back after a drop, nought for none, and the moment in microseconds its
    /// slot is held until once its peer has gone.
    struct Member
    {
        PeerId peer{};
        std::uint8_t slot = kNoSlot;
        std::uint32_t playsFrom = 0;
        std::uint32_t awaitedFrom = 0;
        std::uint64_t reconnectToken = 0;
        std::optional<std::uint64_t> heldUntil;
    };

    /// The first frame of a member that plays no frame yet, or is not awaited yet, later than any frame a match
    /// reaches.
    static constexpr std::uint32_t kNotPlayingYet = 0xFFFFFFFFU;

    /// A match of more slots than a mask of slots has bits breaks a contract.
    explicit Roster(std::uint8_t slotCount);

    /// Lets a client in with the slot it plays and its reconnect token; a slot the match does not have or someone
    /// already holds breaks a contract.
    void admit(PeerId peer, std::uint8_t slot, std::uint64_t reconnectToken = 0);

    /// Lets a client in that catches up with a running match before it plays: a player holding a slot it plays no
    /// frame of yet, or a spectator holding none; a slot the match does not have or someone already holds breaks a
    /// contract.
    void admitJoining(PeerId peer, std::uint8_t slot, std::uint64_t reconnectToken = 0);

    /// Awaits a member catching up from a frame on, its slot in play from then on too if it was not already; any other
    /// member breaks a contract.
    void startPlaying(PeerId peer, std::uint32_t fromFrame);

    /// Hands the slot of the member holding a reconnect token to a new peer, which catches up before it is awaited
    /// again; a token nobody holds breaks a contract.
    void reclaim(std::uint64_t reconnectToken, PeerId peer);

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

    /// Whether a member holds a slot whose inputs the relay does not wait for yet: a player joining a running match, or
    /// one back from a drop, until its first input.
    [[nodiscard]] bool isCatchingUp(PeerId peer) const;

    /// Whether a member holds a slot in play: a player, or one back from a drop; no spectator and no player still
    /// joining.
    [[nodiscard]] bool isInPlay(PeerId peer) const;

    [[nodiscard]] bool isEmpty() const;

    /// The slot a member plays: `kNoSlot` for a spectator and for a peer that is not in the match.
    [[nodiscard]] std::uint8_t slotOf(PeerId peer) const;

    /// The slot of the member holding a reconnect token, its peer gone or not: `kNoSlot` for a token nobody holds and
    /// for nought.
    [[nodiscard]] std::uint8_t slotOfToken(std::uint64_t reconnectToken) const;

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
