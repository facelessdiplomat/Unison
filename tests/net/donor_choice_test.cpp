#include <unison/net/donor_choice.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fixed_round_trips.hpp>

#include <cstdint>

namespace
{

constexpr std::uint8_t kThreeSlots = 3;
constexpr unison::net::PeerId kFirst{1};
constexpr unison::net::PeerId kSecond{2};
constexpr unison::net::PeerId kThird{3};

}

TEST_CASE("the nearest awaited player is the one with the lowest round trip")
{
    unison::net::Roster roster{kThreeSlots};
    roster.admit(kFirst, 0);
    roster.admit(kSecond, 1);
    unison::test::FixedRoundTrips roundTrips;
    roundTrips.set(kFirst, 40'000);
    roundTrips.set(kSecond, 10'000);

    REQUIRE(unison::net::nearestAwaitedPlayer(roster, &roundTrips) == kSecond);
}

TEST_CASE("without a round trip to go by the nearest awaited player is the one in the lowest slot")
{
    unison::net::Roster roster{kThreeSlots};
    roster.admit(kFirst, 1);
    roster.admit(kSecond, 0);

    REQUIRE(unison::net::nearestAwaitedPlayer(roster, nullptr) == kSecond);
}

TEST_CASE("a spectator, a player catching up and a player away are never the nearest awaited player")
{
    unison::net::Roster roster{kThreeSlots};
    roster.admit(kFirst, unison::net::kNoSlot);
    roster.admitJoining(kSecond, 0);
    roster.admit(kThird, 1);
    roster.holdSlotOf(kThird, 1'000);

    REQUIRE_FALSE(unison::net::nearestAwaitedPlayer(roster, nullptr).has_value());
}
