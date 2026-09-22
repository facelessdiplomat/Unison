#pragma once

#include <unison/sim/system_pipeline.hpp>

#include <string_view>

namespace unison::sim
{

/// Advances the physics world of the frame by one tick and writes what moved back into the
/// transforms, walking the entities in the order the registry holds them rather than in the order
/// Jolt happens to wake them.
class PhysicsStep final : public ISystem
{
public:
    void update(Frame& frame, const FrameInputs& inputs) override;

    [[nodiscard]] std::string_view name() const override;
};

}
