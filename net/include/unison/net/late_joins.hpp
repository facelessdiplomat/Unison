#pragma once

#include <unison/net/confirmed_log.hpp>
#include <unison/net/outbox.hpp>
#include <unison/net/protocol.hpp>
#include <unison/net/session_config.hpp>
#include <unison/net/transport.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace unison::net
{

/// The players joining a running match. It asks each one's donor for a snapshot and hands the joiner, over the reliable
/// channel, a welcome naming the snapshot's frame, every chunk of it and then the frames confirmed since that one.
class LateJoins
{
public:
    LateJoins(Outbox& outbox, const ConfirmedLog& confirmedLog, const SessionConfig& config);

    /// Asks a donor for the snapshot of the frame after the newest confirmed one, for a joiner holding a slot and the
    /// reconnect token its welcome will carry.
    void await(PeerId joiner, std::uint8_t slot, std::uint64_t reconnectToken, PeerId donor);

    /// Hands a donor's chunk on to every joiner waiting for that donor, each taking a snapshot from its first chunk on.
    /// A chunk from anyone else or of a frame the log does not hold is left alone.
    void forward(PeerId from, const SnapshotChunk& chunk);

    /// Stops waiting for a joiner that left.
    void forgetJoiner(PeerId joiner);

    /// Asks the next donor, when there is one, for the snapshot of every joiner a donor that left was to send one to,
    /// each taking it from its first chunk on.
    void replaceDonor(PeerId gone, std::optional<PeerId> next);

private:
    struct Join
    {
        PeerId joiner{};
        std::uint8_t slot = kNoSlot;
        std::uint64_t reconnectToken = 0;
        PeerId donor{};
        std::optional<std::uint32_t> snapshotFrame;
        std::uint32_t chunkCount = 0;
        std::uint32_t chunksForwarded = 0;
    };

    void requestSnapshot(PeerId donor);

    void handOn(Join& join, const SnapshotChunk& chunk);

    void welcome(Join& join, const SnapshotChunk& firstChunk);

    void sendConfirmedSince(PeerId joiner, std::uint32_t snapshotFrame);

    [[nodiscard]] static bool isHandedOver(const Join& join);

    Outbox& outbox;
    const ConfirmedLog& confirmedLog;
    SessionConfig config;
    std::uint32_t framesPerDatagram;
    std::vector<Join> joins;
};

}
