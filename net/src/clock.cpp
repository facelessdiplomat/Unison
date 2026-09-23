#include <unison/net/clock.hpp>

namespace unison::net
{

std::uint64_t ManualClock::nowMicroseconds() const
{
    return now;
}

void ManualClock::advance(std::uint64_t microseconds)
{
    now += microseconds;
}

SteadyClock::SteadyClock() : start{std::chrono::steady_clock::now()}
{
}

std::uint64_t SteadyClock::nowMicroseconds() const
{
    const auto elapsed = std::chrono::steady_clock::now() - start;

    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
}

}
