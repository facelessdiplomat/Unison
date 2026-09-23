#pragma once

#include <unison/net/session_config.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

namespace unison::net
{

/// The version of the relay protocol this build speaks; a client speaking another is turned away.
inline constexpr std::uint16_t kProtocolVersion = 4;

/// The most bytes one message may take, small enough to cross the internet in one piece.
inline constexpr std::size_t kMaxDatagramSize = 1200;

/// The slot a welcome names for a spectator, who has none.
inline constexpr std::uint8_t kNoSlot = 0xFF;

/// The most slots a match may have: a mask of slots carries one bit for each of them.
inline constexpr std::uint8_t kMaxSlots = 8;

/// What a confirmation says about one slot's input: that it arrived, or that the relay gave up waiting and
/// repeated the last one. The bits are the ones a frame's inputs carry for its systems.
enum class SlotFlags : std::uint8_t
{
    None = 0,
    Present = 1U << 0U,
    Dropped = 1U << 1U
};

/// Whether a client joins to play in a slot or only to watch.
enum class Role : std::uint8_t
{
    Player,
    Spectator
};

/// Why a peer leaves a room or is sent out of it.
enum class LeaveReason : std::uint8_t
{
    Quit,
    ProtocolMismatch,
    ConfigMismatch,
    RoomFull
};

/// A client asks to join: the protocol it speaks, the hash of the config it would play, the role it wants,
/// and the token of a slot it held before a drop, zero for none.
struct Hello
{
    std::uint16_t protocolVersion = kProtocolVersion;
    std::uint64_t configHash = 0;
    Role role = Role::Player;
    std::uint64_t reconnectToken = 0;
};

/// The relay lets a client in: its slot, the config everyone plays, the frame the match started from, the
/// frame the relay has confirmed so far, and the token that wins the slot back after a drop.
struct Welcome
{
    std::uint8_t slot = 0;
    SessionConfig config;
    std::uint32_t startFrame = 0;
    std::uint32_t confirmedFrame = 0;
    std::uint64_t reconnectToken = 0;
};

/// A client's inputs for `frameCount` frames from `firstFrame` on, `inputSize` bytes each; later ones repeat
/// earlier ones, so a lost message costs nothing.
struct Input
{
    std::uint32_t firstFrame = 0;
    std::uint8_t inputSize = 0;
    std::uint8_t frameCount = 0;
    std::span<const std::byte> inputs;
};

/// The inputs the relay settled for `frameCount` frames from `firstFrame` on, frame after frame, every slot's
/// in slot order, each a flags byte followed by `inputSize` bytes of input; later ones repeat earlier ones, so
/// a lost message costs nothing.
struct Confirmed
{
    std::uint32_t firstFrame = 0;
    std::uint8_t slotCount = 0;
    std::uint8_t inputSize = 0;
    std::uint8_t frameCount = 0;
    std::span<const std::byte> slots;
};

/// The bytes one frame of a confirmation takes: a flags byte and the input for every slot.
[[nodiscard]] constexpr std::size_t confirmedFrameSize(std::uint8_t slotCount, std::uint8_t inputSize)
{
    return std::size_t{slotCount} * (1U + inputSize);
}

/// A client's checksum of a frame it verified.
struct Checksum
{
    std::uint32_t frame = 0;
    std::uint64_t checksum = 0;
};

/// The relay's finding that clients part ways at a frame: one bit per slot that disagrees with the rest.
struct Desync
{
    std::uint32_t frame = 0;
    std::uint8_t minoritySlots = 0;
};

/// The relay asks a client for its snapshot of a verified frame.
struct SnapshotRequest
{
    std::uint32_t frame = 0;
};

/// One piece of a serialised snapshot: its frame, which piece it is of how many, and its bytes.
struct SnapshotChunk
{
    std::uint32_t frame = 0;
    std::uint32_t chunkIndex = 0;
    std::uint32_t chunkCount = 0;
    std::span<const std::byte> bytes;
};

/// A probe of the round trip, stamped with the sender's clock in microseconds.
struct Ping
{
    std::uint64_t sentAt = 0;
};

/// The answer to a ping, as the relay stood when it answered: the stamp the ping carried, the frame confirmed
/// so far, and the frame the relay's clock had due, nought before the first input of the match arrived.
struct Pong
{
    std::uint64_t pingSentAt = 0;
    std::uint32_t confirmedFrame = 0;
    std::uint32_t dueFrame = 0;
};

/// A peer says it is leaving.
struct Leave
{
    LeaveReason reason = LeaveReason::Quit;
};

/// The relay sends a client away.
struct Kick
{
    LeaveReason reason = LeaveReason::Quit;
};

/// Every message of the relay protocol. The order of the alternatives is the type tag on the wire, so a new
/// message is only ever added at the end.
using Message = std::variant<Hello,
                             Welcome,
                             Input,
                             Confirmed,
                             Checksum,
                             Desync,
                             SnapshotRequest,
                             SnapshotChunk,
                             Ping,
                             Pong,
                             Leave,
                             Kick>;

}
