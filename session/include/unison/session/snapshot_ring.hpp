#pragma once

#include <unison/sim/frame.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace unison::session
{

/// The frames a rollback may return to, put aside one per frame in a fixed number of places. A frame
/// takes the place of the one `capacity` frames before it, which is the oldest held while frames are
/// stored in turn; storing a frame again replaces its snapshot.
class SnapshotRing
{
public:
    explicit SnapshotRing(std::uint32_t capacity);

    /// Puts the frame aside under its own number.
    void store(const sim::Frame& frame);

    [[nodiscard]] bool holds(std::uint32_t frame) const;

    /// The snapshot of a frame the ring holds; asking for any other breaks a contract.
    [[nodiscard]] const sim::FrameSnapshot& snapshotAt(std::uint32_t frame) const;

    /// Forgets every frame below the given one.
    void evictBelow(std::uint32_t frame);

private:
    struct Entry
    {
        sim::FrameSnapshot snapshot;
        bool isOccupied = false;
    };

    [[nodiscard]] std::size_t indexOf(std::uint32_t frame) const;

    std::vector<Entry> entries;
};

}
