#include <unison/session/confirmed_inputs.hpp>

namespace unison::session
{

sim::FrameInputs
inputsOfConfirmedFrame(std::span<const std::byte> slots, std::uint8_t slotCount, std::uint8_t inputSize)
{
    const std::size_t stride = 1U + inputSize;
    sim::FrameInputs inputs;

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        const std::span<const std::byte> entry = slots.subspan(slot * stride, stride);

        inputs.setBytes(slot, entry.subspan(1), static_cast<sim::InputFlags>(std::to_integer<std::uint8_t>(entry[0])));
    }

    return inputs;
}

}
