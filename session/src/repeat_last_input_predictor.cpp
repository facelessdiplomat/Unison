#include <unison/session/repeat_last_input_predictor.hpp>

#include <array>
#include <optional>

namespace unison::session
{

namespace
{

constexpr std::array<std::byte, sim::kMaxInputSize> kNeutralInput{};

}

bool RepeatLastInputPredictor::predict(InputBuffer& buffer, std::uint32_t frame, std::size_t slot) const
{
    const std::optional<std::uint32_t> lastConfirmed = buffer.lastConfirmedBefore(frame, slot);

    if (!lastConfirmed.has_value())
    {
        return buffer.store(frame, slot, kNeutralInput, sim::InputFlags::None, InputState::Predicted);
    }

    const sim::FrameInputs& repeated = buffer.inputsAt(*lastConfirmed);

    return buffer.store(frame, slot, repeated.bytesAt(slot), repeated.flagsAt(slot), InputState::Predicted);
}

}
