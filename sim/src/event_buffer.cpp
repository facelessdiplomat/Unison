#include <unison/sim/event_buffer.hpp>

namespace unison::sim
{

std::size_t EventBuffer::size() const
{
    return records.size();
}

const EventKey& EventBuffer::keyAt(std::size_t index) const
{
    UNISON_VERIFY(index < records.size());

    return records[index].key;
}

EventKind EventBuffer::kindAt(std::size_t index) const
{
    UNISON_VERIFY(index < records.size());

    return records[index].kind;
}

void EventBuffer::clear()
{
    records.clear();
    payloads.clear();
    counters.clear();
}

std::uint32_t EventBuffer::nextOrdinal(std::uint32_t typeId)
{
    for (OrdinalCounter& counter : counters)
    {
        if (counter.typeId == typeId)
        {
            return counter.next++;
        }
    }

    counters.push_back(OrdinalCounter{typeId, 1});

    return 0;
}

}
