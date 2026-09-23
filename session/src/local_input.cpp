#include <unison/session/local_input.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>

namespace unison::session
{

LocalInput::LocalInput(std::size_t slot) : localSlot{slot}
{
    UNISON_VERIFY(slot < sim::kMaxSlots);
}

void LocalInput::set(std::span<const std::byte> input)
{
    UNISON_VERIFY(input.size() <= sim::kMaxInputSize);

    if (input.size() > sim::kMaxInputSize)
    {
        return;
    }

    std::ranges::copy(input, latest.begin());
}

bool LocalInput::sampleInto(InputBuffer& buffer, std::uint32_t frame) const
{
    return buffer.store(frame, localSlot, latest, sim::InputFlags::Present, InputState::Predicted);
}

}
