#include <unison/net/network_simulator.hpp>

#include <unison/net/loopback_hub.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace
{

constexpr std::uint64_t kSeed = 20260923;

class NumberCollector final : public unison::net::IMessageReceiver
{
public:
    void receive(unison::net::PeerId, unison::net::Channel, std::span<const std::byte> message) override
    {
        std::uint32_t number = 0;
        std::memcpy(&number, message.data(), sizeof(number));
        received.push_back(number);
    }

    [[nodiscard]] const std::vector<std::uint32_t>& numbers() const
    {
        return received;
    }

private:
    std::vector<std::uint32_t> received;
};

void sendNumbers(unison::net::ITransport& transport,
                 unison::net::PeerId to,
                 unison::net::Channel channel,
                 std::uint32_t count)
{
    for (std::uint32_t number = 0; number < count; ++number)
    {
        transport.send(to, channel, std::as_bytes(std::span{&number, 1}));
    }
}

std::vector<std::uint32_t>
numbersThrough(const unison::net::NetworkConditions& conditions, unison::net::Channel channel, std::uint32_t count)
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& listener = hub.join();
    unison::net::NetworkSimulator network{conditions, kSeed};
    unison::net::SimulatedLink link{sender, network};
    NumberCollector collector;

    sendNumbers(link, listener.id(), channel, count);
    network.advance(conditions.latencyMilliseconds + conditions.jitterMilliseconds);
    listener.poll(collector);

    return collector.numbers();
}

bool isInOrder(const std::vector<std::uint32_t>& numbers)
{
    for (std::size_t index = 0; index < numbers.size(); ++index)
    {
        if (numbers[index] != index)
        {
            return false;
        }
    }

    return true;
}

}

TEST_CASE("a message arrives once its latency has passed and not before")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& sender = hub.join();
    unison::net::LoopbackEndpoint& listener = hub.join();
    unison::net::NetworkSimulator network{unison::net::NetworkConditions{100, 0, 0.0F}, kSeed};
    unison::net::SimulatedLink link{sender, network};
    NumberCollector early;
    NumberCollector onTime;
    sendNumbers(link, listener.id(), unison::net::Channel::Unreliable, 1);

    network.advance(99);
    listener.poll(early);
    network.advance(1);
    listener.poll(onTime);

    REQUIRE(early.numbers().empty());
    REQUIRE(onTime.numbers().size() == 1U);
}

TEST_CASE("the network loses about the share of unreliable messages it is told to")
{
    const std::vector<std::uint32_t> arrived =
        numbersThrough(unison::net::NetworkConditions{20, 0, 0.05F}, unison::net::Channel::Unreliable, 10'000);

    const std::size_t lost = 10'000 - arrived.size();

    REQUIRE(lost >= 450U);
    REQUIRE(lost <= 550U);
}

TEST_CASE("reliable messages are never lost and never overtake one another")
{
    const std::vector<std::uint32_t> arrived =
        numbersThrough(unison::net::NetworkConditions{50, 40, 0.2F}, unison::net::Channel::Reliable, 1'000);

    REQUIRE(arrived.size() == 1'000U);
    REQUIRE(isInOrder(arrived));
}

TEST_CASE("jitter lets unreliable messages overtake one another")
{
    const std::vector<std::uint32_t> arrived =
        numbersThrough(unison::net::NetworkConditions{50, 30, 0.0F}, unison::net::Channel::Unreliable, 200);

    REQUIRE(arrived.size() == 200U);
    REQUIRE_FALSE(isInOrder(arrived));
}

TEST_CASE("two networks started from the same seed decide the same")
{
    const unison::net::NetworkConditions conditions{50, 30, 0.2F};

    const std::vector<std::uint32_t> first = numbersThrough(conditions, unison::net::Channel::Unreliable, 500);
    const std::vector<std::uint32_t> second = numbersThrough(conditions, unison::net::Channel::Unreliable, 500);

    REQUIRE(first == second);
}

TEST_CASE("a simulated link hands over what the transport it wraps received")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& wrapped = hub.join();
    unison::net::LoopbackEndpoint& other = hub.join();
    unison::net::NetworkSimulator network{unison::net::NetworkConditions{}, kSeed};
    unison::net::SimulatedLink link{wrapped, network};
    NumberCollector collector;
    sendNumbers(other, wrapped.id(), unison::net::Channel::Reliable, 1);

    link.poll(collector);

    REQUIRE(collector.numbers() == std::vector<std::uint32_t>{0});
}
