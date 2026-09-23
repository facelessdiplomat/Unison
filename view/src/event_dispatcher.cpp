#include <unison/view/event_dispatcher.hpp>

namespace unison::view
{

void EventDispatcher::dispatch(const session::EventChanges& changes)
{
    for (const sim::EventKey& key : changes.cancelled)
    {
        const Handlers* found = findHandlers(key.typeId);

        if (shown.erase(key) > 0 && found != nullptr && found->cancelled)
        {
            found->cancelled(key);
        }
    }

    for (std::size_t index = 0; index < changes.raised.size(); ++index)
    {
        const sim::EventKey& key = changes.raised.keyAt(index);
        const Handlers* found = findHandlers(key.typeId);

        if (shown.insert(key).second && found != nullptr && found->raised)
        {
            found->raised(key, changes.raised, index);
        }
    }
}

EventDispatcher::Handlers& EventDispatcher::handlersFor(std::uint32_t typeId)
{
    for (Handlers& registered : handlers)
    {
        if (registered.typeId == typeId)
        {
            return registered;
        }
    }

    handlers.push_back(Handlers{typeId, {}, {}});

    return handlers.back();
}

const EventDispatcher::Handlers* EventDispatcher::findHandlers(std::uint32_t typeId) const
{
    for (const Handlers& registered : handlers)
    {
        if (registered.typeId == typeId)
        {
            return &registered;
        }
    }

    return nullptr;
}

}
