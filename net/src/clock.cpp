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

}
