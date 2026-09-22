#pragma once

#include <arena/arena_input.hpp>
#include <arena/assets.hpp>

#include <unison/sim/system_pipeline.hpp>

#include <string_view>

namespace arena
{

/// Turns what a player asked for into what their character is trying to do. The input is quantised:
/// an axis of 127 asks for full speed, a yaw of 65536 steps is a whole turn, and an axis pair longer
/// than one is brought back to one, so running cornerwise is no faster than running straight.
class ApplyInput final : public unison::sim::ISystem
{
public:
    explicit ApplyInput(const PlayerStats& stats);

    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override;

    [[nodiscard]] std::string_view name() const override;

private:
    const PlayerStats& stats;
};

}
