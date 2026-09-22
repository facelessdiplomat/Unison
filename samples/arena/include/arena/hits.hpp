#pragma once

#include <unison/sim/asset_registry.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <string_view>

namespace arena
{

/// Flies every shot on by one tick and settles what it reached on the way. A shot is swept rather
/// than stepped, so it cannot pass through anything between two ticks; a shot that reaches a player
/// takes health off them and is gone, and one that reaches only the world is gone where it struck.
/// A shot that reaches a player and the world in the same tick counts as reaching the player.
class Hits final : public unison::sim::ISystem
{
public:
    explicit Hits(const unison::sim::AssetRegistry& assets);

    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override;

    [[nodiscard]] std::string_view name() const override;

private:
    const unison::sim::AssetRegistry& assets;
};

}
