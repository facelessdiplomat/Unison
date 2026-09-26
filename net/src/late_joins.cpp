#include <unison/net/late_joins.hpp>

#include <unison/net/message_codec.hpp>

#include <algorithm>

namespace unison::net
{

LateJoins::LateJoins(Outbox& outbox, const ConfirmedLog& confirmedLog, const SessionConfig& config)
    : outbox{outbox}, confirmedLog{confirmedLog}, config{config},
      framesPerDatagram{confirmedFramesPerDatagram(config.slotCount, config.inputSize)}
{
}

void LateJoins::await(PeerId joiner, std::uint8_t slot, PeerId donor)
{
    joins.push_back(Join{joiner, slot, donor, std::nullopt, 0, 0});
    outbox.send(donor, Channel::Reliable, SnapshotRequest{confirmedLog.lastFrame() + 1});
}

void LateJoins::forward(PeerId from, const SnapshotChunk& chunk)
{
    if (!confirmedLog.holds(chunk.frame))
    {
        return;
    }

    for (Join& join : joins)
    {
        if (join.donor == from)
        {
            handOn(join, chunk);
        }
    }

    std::erase_if(joins, isHandedOver);
}

void LateJoins::forget(PeerId peer)
{
    std::erase_if(joins, [peer](const Join& join) { return join.joiner == peer || join.donor == peer; });
}

void LateJoins::handOn(Join& join, const SnapshotChunk& chunk)
{
    if (!join.snapshotFrame.has_value() && chunk.chunkIndex == 0)
    {
        welcome(join, chunk);
    }

    if (join.snapshotFrame != chunk.frame || join.chunkCount != chunk.chunkCount)
    {
        return;
    }

    outbox.send(join.joiner, Channel::Reliable, chunk);
    ++join.chunksForwarded;

    if (isHandedOver(join))
    {
        sendConfirmedSince(join.joiner, chunk.frame);
    }
}

void LateJoins::welcome(Join& join, const SnapshotChunk& firstChunk)
{
    join.snapshotFrame = firstChunk.frame;
    join.chunkCount = firstChunk.chunkCount;
    outbox.send(
        join.joiner, Channel::Reliable, Welcome{join.slot, config, firstChunk.frame, confirmedLog.lastFrame(), 0});
}

void LateJoins::sendConfirmedSince(PeerId joiner, std::uint32_t snapshotFrame)
{
    const std::uint32_t lastFrame = confirmedLog.lastFrame();

    for (std::uint32_t first = snapshotFrame + 1; first <= lastFrame; first += framesPerDatagram)
    {
        const std::uint32_t frameCount = std::min(framesPerDatagram, lastFrame - first + 1);
        outbox.send(joiner, Channel::Reliable, confirmationOf(confirmedLog, config, first, frameCount));
    }
}

bool LateJoins::isHandedOver(const Join& join)
{
    return join.snapshotFrame.has_value() && join.chunksForwarded == join.chunkCount;
}

}
