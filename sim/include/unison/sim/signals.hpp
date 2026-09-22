#pragma once

#include <entt/core/type_info.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace unison::sim
{

/// Names one subscription for as long as it lasts, so it can be given up again.
using SubscriptionId = std::uint32_t;

/// A subscriber's side of a signal. The payload arrives erased, because the list holding the
/// subscriptions cannot know every signal type; the handler casts it back, where the type is known.
using SignalHandler = void (*)(void* listener, const void* payload);

/// Synchronous callbacks between systems, delivered in the order they were subscribed and never
/// leaving the simulation. Subscriptions are made once while a game is being put together, so the
/// order is the game's to decide and the same on every client.
class Signals
{
public:
    template <typename T>
    SubscriptionId subscribe(void* listener, SignalHandler handler)
    {
        return add(entt::type_hash<T>::value(), listener, handler);
    }

    template <typename T>
    void emit(const T& payload) const
    {
        deliver(entt::type_hash<T>::value(), &payload);
    }

    void unsubscribe(SubscriptionId subscription);

    [[nodiscard]] std::size_t subscriberCount() const;

private:
    struct Subscription
    {
        entt::id_type typeId = 0;
        SubscriptionId id = 0;
        void* listener = nullptr;
        SignalHandler handler = nullptr;
    };

    SubscriptionId add(entt::id_type typeId, void* listener, SignalHandler handler);

    void deliver(entt::id_type typeId, const void* payload) const;

    std::vector<Subscription> subscriptions;
    SubscriptionId nextId = 1;
};

}
