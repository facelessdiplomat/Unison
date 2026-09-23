#pragma once

#include <unison/session/input_buffer.hpp>

#include <cstddef>
#include <cstdint>

namespace unison::session
{

/// Guesses a slot's input for a frame by repeating the last one confirmed for it before that frame,
/// bytes and flags alike, so a right guess is the confirmed input itself; with none confirmed yet the
/// guess is neutral. Only the buffer knows it is a guess, and it refuses one outside its window.
class RepeatLastInputPredictor
{
public:
    [[nodiscard]] bool predict(InputBuffer& buffer, std::uint32_t frame, std::size_t slot) const;
};

}
