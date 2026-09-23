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

bool isPredictedAndMissingFrom(const sim::EventBuffer& events, std::size_t index, const sim::EventBuffer& other)
{
    return events.kindAt(index) == sim::EventKind::Predicted && !raises(other, events.keyAt(index));
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

    for (std::size_t index = 0; index < events.size(); ++index)
    {
        if (isPredictedAndMissingFrom(events, index, entry.events))
        {
            changes.raised.append(events, index);
        }
    }

    for (std::size_t index = 0; index < entry.events.size(); ++index)
    {
        if (isPredictedAndMissingFrom(entry.events, index, events))
        {
            changes.cancelled.push_back(entry.events.keyAt(index));
        }
    }

    entry.frameNumber = frameNumber;
    entry.events = events;
}

void EventHistory::release(std::uint32_t frameNumber, EventChanges& changes) const
{
    const Entry& entry = entries[frameNumber % entries.size()];

    UNISON_VERIFY(entry.frameNumber == frameNumber);

    for (std::size_t index = 0; index < entry.events.size(); ++index)
    {
        if (entry.events.kindAt(index) == sim::EventKind::VerifiedOnly)
        {
            changes.raised.append(entry.events, index);
        }
    }
}

}
