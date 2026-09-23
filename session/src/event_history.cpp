#include <unison/session/event_history.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>
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

void noteRaised(EventChanges& changes, const sim::EventBuffer& events, std::size_t index)
{
    const auto cancelled = std::ranges::find(changes.cancelled, events.keyAt(index));

    if (cancelled != changes.cancelled.end())
    {
        changes.cancelled.erase(cancelled);

        return;
    }

    changes.raised.append(events, index);
}

void noteCancelled(EventChanges& changes, const sim::EventKey& key)
{
    if (raises(changes.raised, key))
    {
        changes.raised.remove(key);

        return;
    }

    changes.cancelled.push_back(key);
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
            noteRaised(changes, events, index);
        }
    }

    for (std::size_t index = 0; index < entry.events.size(); ++index)
    {
        if (isPredictedAndMissingFrom(entry.events, index, events))
        {
            noteCancelled(changes, entry.events.keyAt(index));
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
