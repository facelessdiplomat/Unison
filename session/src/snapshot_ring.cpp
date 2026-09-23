#include <unison/session/snapshot_ring.hpp>

#include <unison/core/contract.hpp>

namespace unison::session
{

SnapshotRing::SnapshotRing(std::uint32_t capacity) : entries(capacity)
{
    UNISON_VERIFY(capacity > 0);
}

void SnapshotRing::store(const sim::Frame& frame)
{
    Entry& entry = entries[indexOf(frame.frameNumber)];

    sim::takeSnapshot(frame, entry.snapshot);
    entry.isOccupied = true;
}

bool SnapshotRing::holds(std::uint32_t frame) const
{
    const Entry& entry = entries[indexOf(frame)];

    return entry.isOccupied && entry.snapshot.frameNumber == frame;
}

const sim::FrameSnapshot& SnapshotRing::snapshotAt(std::uint32_t frame) const
{
    UNISON_VERIFY(holds(frame));

    return entries[indexOf(frame)].snapshot;
}

void SnapshotRing::evictBelow(std::uint32_t frame)
{
    for (Entry& entry : entries)
    {
        if (entry.snapshot.frameNumber < frame)
        {
            entry.isOccupied = false;
        }
    }
}

std::size_t SnapshotRing::indexOf(std::uint32_t frame) const
{
    return frame % entries.size();
}

}
