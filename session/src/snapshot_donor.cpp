#include <unison/session/snapshot_donor.hpp>

#include <unison/session/snapshot_chunks.hpp>
#include <unison/session/snapshot_serializer.hpp>

namespace unison::session
{

SnapshotDonor::SnapshotDonor(net::Outbox& outbox, net::PeerId relay) : outbox{outbox}, relay{relay}
{
}

void SnapshotDonor::request(std::uint32_t frame)
{
    requested = frame;
}

void SnapshotDonor::frameVerified(const VerifiedFrame& frame)
{
    if (!requested.has_value() || frame.frameNumber < *requested)
    {
        return;
    }

    requested.reset();
    serializeSnapshot(frame.snapshot, serialized);

    for (const net::SnapshotChunk& chunk : chunksOf(frame.frameNumber, serialized))
    {
        outbox.send(relay, net::Channel::Reliable, chunk);
    }
}

}
