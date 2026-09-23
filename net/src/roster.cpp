#include <unison/net/roster.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>

namespace unison::net
{

Roster::Roster(std::uint8_t slotCount) : slotCount{slotCount}
{
    UNISON_VERIFY(slotCount <= kMaxSlots);
}

void Roster::admit(PeerId peer, std::uint8_t slot)
{
    UNISON_VERIFY(slot == kNoSlot || (slot < slotCount && !isHeld(slot)));

    admitted.push_back(Member{peer, slot});
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

std::uint8_t Roster::slotOf(PeerId peer) const
{
    const auto found = std::ranges::find(admitted, peer, &Member::peer);

    return found == admitted.end() ? kNoSlot : found->slot;
}

std::uint8_t Roster::slotsInPlay() const
{
    std::uint8_t inPlay = 0;

    for (const Member& member : admitted)
    {
        if (member.slot != kNoSlot)
        {
            inPlay = static_cast<std::uint8_t>(inPlay | (1U << member.slot));
        }
    }

    return inPlay;
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
