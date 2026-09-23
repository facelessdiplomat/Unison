#include <unison/net/millisecond_timer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

namespace
{

constexpr int kSleeps = 20;
constexpr std::chrono::milliseconds kSleep{1};
constexpr std::chrono::milliseconds kLongestFairSleep{5};

}

TEST_CASE("while a millisecond timer lives, a sleep of a millisecond lasts a few at most")
{
    const unison::net::MillisecondTimer timer;
    const auto start = std::chrono::steady_clock::now();

    for (int sleep = 0; sleep < kSleeps; ++sleep)
    {
        std::this_thread::sleep_for(kSleep);
    }

    const auto slept = std::chrono::steady_clock::now() - start;

    REQUIRE(slept < kSleeps * kLongestFairSleep);
}
