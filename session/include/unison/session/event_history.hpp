#pragma once

#include <unison/sim/event_buffer.hpp>

#include <cstdint>
#include <vector>

namespace unison::session
{

/// What playing and verifying frames changed for the view: the events it must now show, predicted ones as
/// soon as they are raised and the others once their frame is verified, and the ones a replay took back.
struct EventChanges
{
    sim::EventBuffer raised;
    std::vector<sim::EventKey> cancelled;
};

/// The events every frame the session may still play again raised the last time it was played, so a
/// replay can tell which predicted events are new and which it took back. A frame recorded in the place
/// of an older one counts as played for the first time.
class EventHistory
{
public:
    explicit EventHistory(std::uint32_t capacity);

    /// Keeps the events a frame raised in place of what it raised when last played, and adds to the
    /// changes its predicted events that are new and those it no longer raises.
    void record(std::uint32_t frameNumber, const sim::EventBuffer& events, EventChanges& changes);

    /// Adds to the changes the events of a verified frame that waited for it, as the frame raised them when
    /// it was last played. The frame must still be in the history.
    void release(std::uint32_t frameNumber, EventChanges& changes) const;

private:
    struct Entry
    {
        std::uint32_t frameNumber = 0;
        sim::EventBuffer events;
    };

    std::vector<Entry> entries;
};

}
