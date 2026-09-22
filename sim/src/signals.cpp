#include <unison/sim/signals.hpp>

#include <unison/core/contract.hpp>

namespace unison::sim
{

void Signals::unsubscribe(SubscriptionId subscription)
{
    for (Subscription& entry : subscriptions)
    {
        if (entry.id == subscription)
        {
            entry.handler = nullptr;

            return;
        }
    }
}

std::size_t Signals::subscriberCount() const
{
    std::size_t living = 0;

    for (const Subscription& entry : subscriptions)
    {
        living += entry.handler == nullptr ? 0U : 1U;
    }

    return living;
}

SubscriptionId Signals::add(entt::id_type typeId, void* listener, SignalHandler handler)
{
    UNISON_VERIFY(handler != nullptr);

    const SubscriptionId subscription = nextId++;

    subscriptions.push_back(Subscription{typeId, subscription, listener, handler});

    return subscription;
}

void Signals::deliver(entt::id_type typeId, const void* payload) const
{
    for (const Subscription& entry : subscriptions)
    {
        if (entry.typeId == typeId && entry.handler != nullptr)
        {
            entry.handler(entry.listener, payload);
        }
    }
}

}
