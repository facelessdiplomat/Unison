#include <unison/sim/signals.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace
{

struct Hit
{
    std::uint32_t damage = 0;
};

struct Healed
{
    std::uint32_t amount = 0;
};

class Listener
{
public:
    Listener(std::string listenerName, std::vector<std::string>& log) : listenerName{std::move(listenerName)}, log{log}
    {
    }

    void subscribeTo(unison::sim::Signals& signals)
    {
        subscription = signals.subscribe<Hit>(this,
                                              [](void* context, const void* payload)
                                              {
                                                  auto* listener = static_cast<Listener*>(context);

                                                  listener->log.push_back(listener->listenerName);
                                                  listener->lastDamage = static_cast<const Hit*>(payload)->damage;
                                              });
    }

    [[nodiscard]] unison::sim::SubscriptionId id() const
    {
        return subscription;
    }

    [[nodiscard]] std::uint32_t damage() const
    {
        return lastDamage;
    }

private:
    std::string listenerName;
    std::vector<std::string>& log;
    unison::sim::SubscriptionId subscription = 0;
    std::uint32_t lastDamage = 0;
};

}

TEST_CASE("subscribers hear a signal in the order they subscribed")
{
    std::vector<std::string> log;
    unison::sim::Signals signals;

    Listener first{"first", log};
    Listener second{"second", log};

    first.subscribeTo(signals);
    second.subscribeTo(signals);

    signals.emit(Hit{7});

    REQUIRE(log == std::vector<std::string>{"first", "second"});
}

TEST_CASE("a subscriber hears the payload it was sent")
{
    std::vector<std::string> log;
    unison::sim::Signals signals;
    Listener listener{"only", log};

    listener.subscribeTo(signals);
    signals.emit(Hit{42});

    REQUIRE(listener.damage() == 42U);
}

TEST_CASE("a subscriber that unsubscribed hears nothing")
{
    std::vector<std::string> log;
    unison::sim::Signals signals;

    Listener staying{"staying", log};
    Listener leaving{"leaving", log};

    staying.subscribeTo(signals);
    leaving.subscribeTo(signals);
    signals.unsubscribe(leaving.id());

    signals.emit(Hit{1});

    REQUIRE(log == std::vector<std::string>{"staying"});
}

TEST_CASE("a subscriber hears only the signal it subscribed to")
{
    std::vector<std::string> log;
    unison::sim::Signals signals;
    Listener listener{"only", log};

    listener.subscribeTo(signals);
    signals.emit(Healed{5});

    REQUIRE(log.empty());
}

TEST_CASE("a signal nobody subscribed to passes without trace")
{
    unison::sim::Signals signals;

    signals.emit(Healed{5});

    REQUIRE(signals.subscriberCount() == 0U);
}
