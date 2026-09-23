#include <unison/sim/event_buffer.hpp>

namespace unison::sim
{

void EventBuffer::append(const EventBuffer& source, std::size_t index)
{
    UNISON_VERIFY(&source != this);
    UNISON_VERIFY(index < source.records.size());

    if (&source == this || index >= source.records.size())
    {
        return;
    }

    const Record& record = source.records[index];
    const auto offset = static_cast<std::uint32_t>(payloads.size());
    const auto first = source.payloads.begin() + record.offset;

    payloads.insert(payloads.end(), first, first + record.size);
    records.push_back(Record{record.key, record.kind, offset, record.size});
}

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
