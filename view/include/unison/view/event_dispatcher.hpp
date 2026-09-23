#pragma once

#include <unison/session/event_history.hpp>
#include <unison/sim/event_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <utility>
#include <vector>

namespace unison::view
{

/// Hands the events a session raised and took back to the handlers the host registered for their types,
/// one handler of each kind per type. A raised event reaches its handler once however often it is handed
/// over, and a cancellation reaches the host only for an event it was shown.
class EventDispatcher
{
public:
    template <typename T>
    void on(std::function<void(const sim::EventKey&, const T&)> handler)
    {
        handlersFor(sim::EventTraits<T>::typeId).raised =
            [typed = std::move(handler)](const sim::EventKey& key, const sim::EventBuffer& events, std::size_t index)
        {
            typed(key, events.payloadAt<T>(index));
        };
    }

    template <typename T>
    void onCancelled(std::function<void(const sim::EventKey&)> handler)
    {
        handlersFor(sim::EventTraits<T>::typeId).cancelled = std::move(handler);
    }

    /// Delivers the cancellations and then the raised events of a batch of changes.
    void dispatch(const session::EventChanges& changes);

    /// Forgets the events of the frames below the given one, which can no longer be taken back, so the events
    /// it remembers stop growing; an event of those frames handed over again would be shown again.
    void forgetBelow(std::uint32_t frame);

private:
    struct Handlers
    {
        std::uint32_t typeId = 0;
        std::function<void(const sim::EventKey&, const sim::EventBuffer&, std::size_t)> raised;
        std::function<void(const sim::EventKey&)> cancelled;
    };

    [[nodiscard]] Handlers& handlersFor(std::uint32_t typeId);

    [[nodiscard]] const Handlers* findHandlers(std::uint32_t typeId) const;

    std::vector<Handlers> handlers;
    std::set<sim::EventKey> shown;
};

}
