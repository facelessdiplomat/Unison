#pragma once

#include <unison/sim/system_pipeline.hpp>

#include <string_view>

namespace arena
{

/// Counts down whatever was given a limited stay in the world and takes it away on the tick its
/// count runs out, so a shot that reaches nothing does not fly for the rest of the match.
class Lifetimes final : public unison::sim::ISystem
{
public:
    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override;

    [[nodiscard]] std::string_view name() const override;
};

}
