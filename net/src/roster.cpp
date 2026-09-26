#include <unison/net/roster.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>

namespace unison::net
{

Roster::Roster(std::uint8_t slotCount) : slotCount{slotCount}
{
    UNISON_VERIFY(slotCount <= kMaxSlots);
}

void Roster::admit(PeerId peer, std::uint8_t slot, std::uint64_t reconnectToken)
{
    UNISON_VERIFY(slot == kNoSlot || (slot < slotCount && !isHeld(slot)));

    admitted.push_back(Member{peer, slot, 0, reconnectToken, std::nullopt});
}

void Roster::admitJoining(PeerId peer, std::uint8_t slot, std::uint64_t reconnectToken)
{
    UNISON_VERIFY(slot < slotCount && !isHeld(slot));

    admitted.push_back(Member{peer, slot, kNotPlayingYet, reconnectToken, std::nullopt});
}

void Roster::startPlaying(PeerId peer, std::uint32_t fromFrame)
{
    const auto member = std::ranges::find(admitted, peer, &Member::peer);
    const bool isJoiningMember = member != admitted.end() && member->playsFrom == kNotPlayingYet;

    UNISON_VERIFY(isJoiningMember);

    if (isJoiningMember)
    {
        member->playsFrom = fromFrame;
    }
}

void Roster::remove(PeerId peer)
{
    std::erase_if(admitted, [peer](const Member& member) { return member.peer == peer; });
}

void Roster::holdSlotOf(PeerId peer, std::uint64_t until)
{
    const auto member = std::ranges::find(admitted, peer, &Member::peer);

    if (member != admitted.end())
    {
        member->heldUntil = until;
    }
}

void Roster::releaseHeldSlots(std::uint64_t now)
{
    std::erase_if(admitted,
                  [now](const Member& member) { return member.heldUntil.has_value() && *member.heldUntil <= now; });
}

std::uint8_t Roster::freeSlot() const
{
    for (std::uint8_t slot = 0; slot < slotCount; ++slot)
    {
        if (!isHeld(slot))
        {
            return slot;
        }
    }

    return kNoSlot;
}

bool Roster::isMember(PeerId peer) const
{
    return std::ranges::find(admitted, peer, &Member::peer) != admitted.end();
}

bool Roster::isJoining(PeerId peer) const
{
    const auto member = std::ranges::find(admitted, peer, &Member::peer);

    return member != admitted.end() && member->playsFrom == kNotPlayingYet;
}

bool Roster::isEmpty() const
{
    return admitted.empty();
}

std::uint8_t Roster::slotOf(PeerId peer) const
{
    const auto found = std::ranges::find(admitted, peer, &Member::peer);

    return found == admitted.end() ? kNoSlot : found->slot;
}

std::uint8_t Roster::slotsInPlayAt(std::uint32_t frame) const
{
    std::uint8_t inPlay = 0;

    for (const Member& member : admitted)
    {
        if (member.slot != kNoSlot && member.playsFrom <= frame)
        {
            inPlay = static_cast<std::uint8_t>(inPlay | (1U << member.slot));
        }
    }

    return inPlay;
}

std::uint8_t Roster::slotsAwaitedAt(std::uint32_t frame) const
{
    std::uint8_t awaited = slotsInPlayAt(frame);

    for (const Member& member : admitted)
    {
        if (member.slot != kNoSlot && member.heldUntil.has_value())
        {
            awaited = static_cast<std::uint8_t>(awaited & ~(1U << member.slot));
        }
    }

    return awaited;
}

std::span<const Roster::Member> Roster::members() const
{
    return admitted;
}

bool Roster::isHeld(std::uint8_t slot) const
{
    return std::ranges::find(admitted, slot, &Member::slot) != admitted.end();
}

}
