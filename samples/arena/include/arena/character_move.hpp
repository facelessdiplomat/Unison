#pragma once

#include <arena/assets.hpp>

#include <unison/sim/system_pipeline.hpp>

#include <string_view>

namespace arena
{

/// Drives the character of every player from what they are trying to do: the way they want to go
/// becomes the way they walk, the match pulls them down every tick, and the jump button lifts them
/// only while they still have the ground under their feet.
class CharacterMove final : public unison::sim::ISystem
{
public:
    explicit CharacterMove(const PlayerStats& stats);

    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override;

    [[nodiscard]] std::string_view name() const override;

private:
    const PlayerStats& stats;
};

}
