#pragma once

#include <unison/sim/asset_registry.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <string_view>

namespace arena
{

/// Fires the weapon of every player holding the trigger whose weapon has cooled down, sending a
/// shot from their eyes along the way they are facing and telling the view about it. A shot is a
/// swept entity rather than a body, so it starts clear of the player who fired it.
class Weapons final : public unison::sim::ISystem
{
public:
    explicit Weapons(const unison::sim::AssetRegistry& assets);

    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs& inputs) override;

    [[nodiscard]] std::string_view name() const override;

private:
    const unison::sim::AssetRegistry& assets;
};

}
