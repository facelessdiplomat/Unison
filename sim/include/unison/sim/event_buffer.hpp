#pragma once

#include <unison/core/asset_id.hpp>
#include <unison/core/contract.hpp>
#include <unison/core/raw_value.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace unison::sim
{

/// When the view may show an event. A verified-only event waits until its frame can no longer be
/// rolled back; a predicted one is shown at once and cancelled by key if a rollback removes it.
enum class EventKind : std::uint8_t
{
    VerifiedOnly,
    Predicted
};

/// Names one raised event for as long as anyone cares about it, across a rollback and across the
/// wire: which frame raised it, which type it is, and which of that type within the frame.
struct EventKey
{
    std::uint32_t frame = 0;
    std::uint32_t typeId = 0;
    std::uint32_t ordinal = 0;

    bool operator==(const EventKey& other) const = default;
};

/// What the engine needs to know about one event type. A game declares it with UNISON_EVENT; there
/// is no primary definition, so an event type that was never declared fails to compile.
template <typename T>
struct EventTraits;

/// The events raised while one tick runs, in the order they were raised. A frame owns one and
/// clears it every tick, so an event never outlives the tick that raised it.
class EventBuffer
{
public:
    template <RawValue T>
    EventKey raise(std::uint32_t frame, const T& payload)
    {
        const EventKey key{frame, EventTraits<T>::typeId, nextOrdinal(EventTraits<T>::typeId)};
        const auto offset = static_cast<std::uint32_t>(payloads.size());

        payloads.resize(payloads.size() + sizeof(T));
        std::memcpy(payloads.data() + offset, &payload, sizeof(T));
        records.push_back(Record{key, EventTraits<T>::kind, offset, sizeof(T)});

        return key;
    }

    template <RawValue T>
    [[nodiscard]] T payloadAt(std::size_t index) const
    {
        UNISON_VERIFY(index < records.size());
        UNISON_VERIFY(records[index].key.typeId == EventTraits<T>::typeId);

        T payload{};
        std::memcpy(&payload, payloads.data() + records[index].offset, sizeof(T));

        return payload;
    }

    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] const EventKey& keyAt(std::size_t index) const;

    [[nodiscard]] EventKind kindAt(std::size_t index) const;

    void clear();

private:
    struct Record
    {
        EventKey key;
        EventKind kind = EventKind::VerifiedOnly;
        std::uint32_t offset = 0;
        std::uint32_t size = 0;
    };

    struct OrdinalCounter
    {
        std::uint32_t typeId = 0;
        std::uint32_t next = 0;
    };

    std::uint32_t nextOrdinal(std::uint32_t typeId);

    std::vector<Record> records;
    std::vector<std::byte> payloads;
    std::vector<OrdinalCounter> counters;
};

}

/// Declares an event type and when the view may show it. Written at namespace scope outside any
/// namespace, naming the type as the rest of the program sees it, because that spelling becomes the
/// identifier the event travels under.
#define UNISON_EVENT(Type, Kind)                                                                                       \
    template <>                                                                                                        \
    struct unison::sim::EventTraits<Type>                                                                              \
    {                                                                                                                  \
        static constexpr ::unison::sim::EventKind kind = (Kind);                                                       \
        static constexpr std::uint32_t typeId = static_cast<std::uint32_t>(::unison::makeAssetId(#Type));              \
    }
