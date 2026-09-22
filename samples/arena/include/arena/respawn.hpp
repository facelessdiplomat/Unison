#pragma once

#include <unison/sim/asset_registry.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <string_view>

namespace arena
{

/// Sees a player through the end of one life and the start of the next: a player whose health runs
/// out is taken out of the world and given a timer, and when the timer ends they come back whole at
/// a spawn point the match draws for them.
class Respawn final : public unison::sim::ISystem
{
public:
    explicit Respawn(const unison::sim::AssetRegistry& assets);

    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override;

    [[nodiscard]] std::string_view name() const override;

private:
    const unison::sim::AssetRegistry& assets;
};

}
