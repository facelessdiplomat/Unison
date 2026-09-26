#include <unison/net/roster.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstdint>

namespace
{

constexpr std::uint8_t kTwoSlots = 2;
constexpr unison::net::PeerId kFirst{1};
constexpr unison::net::PeerId kSecond{2};
constexpr unison::net::PeerId kThird{3};

}

TEST_CASE("players take the lowest slots nobody holds")
{
    unison::net::Roster roster{kTwoSlots};
    const std::uint8_t first = roster.freeSlot();
    roster.admit(kFirst, first);

    const std::uint8_t second = roster.freeSlot();

    REQUIRE(first == 0U);
    REQUIRE(second == 1U);
}

TEST_CASE("a roster whose slots are all held has no free slot")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);
    roster.admit(kSecond, 1);

    REQUIRE(roster.freeSlot() == unison::net::kNoSlot);
}

TEST_CASE("a spectator is a member without a slot")
{
    unison::net::Roster roster{kTwoSlots};

    roster.admit(kFirst, unison::net::kNoSlot);

    REQUIRE(roster.isMember(kFirst));
    REQUIRE(roster.slotOf(kFirst) == unison::net::kNoSlot);
    REQUIRE(roster.freeSlot() == 0U);
}

TEST_CASE("a peer never let in is no member and plays no slot")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);

    REQUIRE_FALSE(roster.isMember(kSecond));
    REQUIRE(roster.slotOf(kSecond) == unison::net::kNoSlot);
}

TEST_CASE("the slots in play have one bit for every slot a player holds")
{
    unison::net::Roster roster{3};
    roster.admit(kFirst, 0);
    roster.admit(kSecond, unison::net::kNoSlot);
    roster.admit(kThird, 2);

    REQUIRE(roster.slotsInPlayAt(1) == 0b101U);
}

TEST_CASE("members are listed in the order they were let in")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kSecond, 1);
    roster.admit(kThird, unison::net::kNoSlot);
    roster.admit(kFirst, 0);

    const auto members = roster.members();

    REQUIRE(members.size() == 3U);
    REQUIRE(members[0].peer == kSecond);
    REQUIRE(members[1].peer == kThird);
    REQUIRE(members[2].peer == kFirst);
    REQUIRE(members[2].slot == 0U);
}

TEST_CASE("letting a player into a slot someone holds breaks a contract")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);
    const unison::test::FatalHandlerProbe probe;

    roster.admit(kSecond, 0);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("letting a player into a slot the match does not have breaks a contract")
{
    unison::net::Roster roster{kTwoSlots};
    const unison::test::FatalHandlerProbe probe;

    roster.admit(kFirst, kTwoSlots);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a roster for more slots than a mask of slots has bits breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;

    const unison::net::Roster roster{unison::net::kMaxSlots + 1};

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a member who leaves is no member and frees the slot it held")
{
    unison::net::Roster roster{2};
    roster.admit(unison::net::PeerId{1}, 0);
    roster.admit(unison::net::PeerId{2}, 1);

    roster.remove(unison::net::PeerId{1});

    REQUIRE_FALSE(roster.isMember(unison::net::PeerId{1}));
    REQUIRE(roster.freeSlot() == 0U);
    REQUIRE(roster.slotsInPlayAt(1) == 0b10U);
}

TEST_CASE("a roster everyone has left is empty")
{
    unison::net::Roster roster{2};
    roster.admit(unison::net::PeerId{1}, 0);
    roster.admit(unison::net::PeerId{2}, unison::net::kNoSlot);
    const bool wasEmpty = roster.isEmpty();

    roster.remove(unison::net::PeerId{1});
    roster.remove(unison::net::PeerId{2});

    REQUIRE_FALSE(wasEmpty);
    REQUIRE(roster.isEmpty());
}

TEST_CASE("taking out a peer who never came in changes nothing")
{
    unison::net::Roster roster{2};
    roster.admit(unison::net::PeerId{1}, 0);

    roster.remove(unison::net::PeerId{9});

    REQUIRE(roster.isMember(unison::net::PeerId{1}));
    REQUIRE(roster.members().size() == 1U);
}

TEST_CASE("a member still joining holds its slot but none of the slots in play")
{
    constexpr std::uint8_t kThreeSlots = 3;
    unison::net::Roster roster{kThreeSlots};
    roster.admit(kFirst, 0);

    roster.admitJoining(kSecond, 1);

    REQUIRE(roster.isMember(kSecond));
    REQUIRE(roster.isCatchingUp(kSecond));
    REQUIRE(roster.slotOf(kSecond) == 1U);
    REQUIRE(roster.freeSlot() == 2U);
    REQUIRE(roster.slotsInPlayAt(1) == 0b001U);
}

TEST_CASE("a member that starts playing puts its slot in play from the frame it starts at")
{
    constexpr std::uint8_t kThreeSlots = 3;
    unison::net::Roster roster{kThreeSlots};
    roster.admit(kFirst, 0);
    roster.admitJoining(kSecond, 1);

    roster.startPlaying(kSecond, 5);

    REQUIRE_FALSE(roster.isCatchingUp(kSecond));
    REQUIRE(roster.slotsInPlayAt(4) == 0b001U);
    REQUIRE(roster.slotsInPlayAt(5) == 0b011U);
}

TEST_CASE("letting a joining player into a slot someone holds breaks a contract")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);
    const unison::test::FatalHandlerProbe probe;

    roster.admitJoining(kSecond, 0);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("letting a joining player into a slot the match does not have breaks a contract")
{
    unison::net::Roster roster{kTwoSlots};
    const unison::test::FatalHandlerProbe probe;

    roster.admitJoining(kFirst, kTwoSlots);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a spectator joining a running match catches up without a slot and never comes into play")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);

    roster.admitJoining(kSecond, unison::net::kNoSlot);

    REQUIRE(roster.isMember(kSecond));
    REQUIRE(roster.slotOf(kSecond) == unison::net::kNoSlot);
    REQUIRE(roster.freeSlot() == 1U);
    REQUIRE_FALSE(roster.isInPlay(kSecond));
    REQUIRE(roster.slotsInPlayAt(1) == 0b01U);
}

TEST_CASE("putting in play a member that is not catching up breaks a contract")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);
    const unison::test::FatalHandlerProbe probe;

    roster.startPlaying(kFirst, 3);
    roster.startPlaying(kSecond, 3);

    REQUIRE(probe.failureCount() == 2U);
}

TEST_CASE("a member is let in with the token that wins its slot back")
{
    unison::net::Roster roster{kTwoSlots};

    roster.admit(kFirst, 0, 77);
    roster.admitJoining(kSecond, 1, 78);

    REQUIRE(roster.members()[0].reconnectToken == 77U);
    REQUIRE(roster.members()[1].reconnectToken == 78U);
}

TEST_CASE("a member whose slot is held keeps it in play but is not awaited")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);
    roster.admit(kSecond, 1);

    roster.holdSlotOf(kSecond, 1'000);

    REQUIRE(roster.isMember(kSecond));
    REQUIRE(roster.freeSlot() == unison::net::kNoSlot);
    REQUIRE(roster.slotsInPlayAt(1) == 0b11U);
    REQUIRE(roster.slotsAwaitedAt(1) == 0b01U);
}

TEST_CASE("held slots are released once the moment they were held until has come, and no sooner")
{
    constexpr std::uint8_t kThreeSlots = 3;
    unison::net::Roster roster{kThreeSlots};
    roster.admit(kFirst, 0);
    roster.admit(kSecond, 1);
    roster.admit(kThird, 2);
    roster.holdSlotOf(kSecond, 1'000);
    roster.holdSlotOf(kThird, 2'000);

    roster.releaseHeldSlots(999);
    const bool isHeldBefore = roster.isMember(kSecond);
    roster.releaseHeldSlots(1'000);

    REQUIRE(isHeldBefore);
    REQUIRE_FALSE(roster.isMember(kSecond));
    REQUIRE(roster.isMember(kThird));
    REQUIRE(roster.freeSlot() == 1U);
}

TEST_CASE("a slot is found by the reconnect token of the member holding it, held or not")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0, 77);
    roster.admit(kSecond, 1, 78);
    roster.holdSlotOf(kSecond, 1'000);

    REQUIRE(roster.slotOfToken(77) == 0U);
    REQUIRE(roster.slotOfToken(78) == 1U);
}

TEST_CASE("no slot is found for a token nobody holds, nor for nought")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);
    roster.admit(kSecond, 1, 78);

    REQUIRE(roster.slotOfToken(79) == unison::net::kNoSlot);
    REQUIRE(roster.slotOfToken(0) == unison::net::kNoSlot);
}

TEST_CASE("a slot reclaimed with its token belongs to the new peer, in play but not awaited until it plays again")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0, 77);
    roster.admit(kSecond, 1, 78);
    roster.holdSlotOf(kSecond, 1'000);

    roster.reclaim(78, kThird);

    REQUIRE_FALSE(roster.isMember(kSecond));
    REQUIRE(roster.slotOf(kThird) == 1U);
    REQUIRE(roster.isInPlay(kThird));
    REQUIRE(roster.isCatchingUp(kThird));
    REQUIRE(roster.slotsInPlayAt(5) == 0b11U);
    REQUIRE(roster.slotsAwaitedAt(5) == 0b01U);
}

TEST_CASE("a member back from a drop is awaited again from the frame it plays from, its slot in play all along")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0, 77);
    roster.admit(kSecond, 1, 78);
    roster.holdSlotOf(kSecond, 1'000);
    roster.reclaim(78, kThird);

    roster.startPlaying(kThird, 10);

    REQUIRE_FALSE(roster.isCatchingUp(kThird));
    REQUIRE(roster.slotsInPlayAt(9) == 0b11U);
    REQUIRE(roster.slotsAwaitedAt(9) == 0b01U);
    REQUIRE(roster.slotsAwaitedAt(10) == 0b11U);
}

TEST_CASE("reclaiming a slot with a token nobody holds breaks a contract")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0, 77);
    const unison::test::FatalHandlerProbe probe;

    roster.reclaim(79, kSecond);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a player is in play, and neither a spectator nor a joiner still catching up is")
{
    unison::net::Roster roster{kTwoSlots};
    roster.admit(kFirst, 0);
    roster.admitJoining(kSecond, 1);
    roster.admit(kThird, unison::net::kNoSlot);

    REQUIRE(roster.isInPlay(kFirst));
    REQUIRE_FALSE(roster.isInPlay(kSecond));
    REQUIRE_FALSE(roster.isInPlay(kThird));
}
