#pragma once

#include <unison/sim/asset_registry.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <string_view>

namespace arena
{

/// Keeps the match itself: the players warm up until the match begins, every kill made while it is
/// being played is counted for the player who made it, and the first to the limit ends it. A kill
/// made before the match begins or after it ends counts for nobody.
class MatchRules final : public unison::sim::ISystem
{
public:
    explicit MatchRules(const unison::sim::AssetRegistry& assets);

    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override;

    [[nodiscard]] std::string_view name() const override;

private:
    const unison::sim::AssetRegistry& assets;
};

}
