#include <unison/session/event_history.hpp>

#include <unison/core/contract.hpp>

#include <cstddef>

namespace unison::session
{

namespace
{

bool raises(const sim::EventBuffer& events, const sim::EventKey& key)
{
    for (std::size_t index = 0; index < events.size(); ++index)
    {
        if (events.keyAt(index) == key)
        {
            return true;
        }
    }

    return false;
}

void addPredictedEventsMissingFrom(const sim::EventBuffer& events,
                                   const sim::EventBuffer& other,
                                   std::vector<sim::EventKey>& keys)
{
    for (std::size_t index = 0; index < events.size(); ++index)
    {
        if (events.kindAt(index) == sim::EventKind::Predicted && !raises(other, events.keyAt(index)))
        {
            keys.push_back(events.keyAt(index));
        }
    }
}

}

EventHistory::EventHistory(std::uint32_t capacity) : entries(capacity)
{
    UNISON_VERIFY(capacity > 0);
}

void EventHistory::record(std::uint32_t frameNumber, const sim::EventBuffer& events, EventChanges& changes)
{
    Entry& entry = entries[frameNumber % entries.size()];

    if (entry.frameNumber != frameNumber)
    {
        entry.events.clear();
    }

    addPredictedEventsMissingFrom(events, entry.events, changes.raised);
    addPredictedEventsMissingFrom(entry.events, events, changes.cancelled);

    entry.frameNumber = frameNumber;
    entry.events = events;
}

}
