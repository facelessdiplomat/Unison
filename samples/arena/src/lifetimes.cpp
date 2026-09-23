#include <arena/lifetimes.hpp>

#include <arena/components.hpp>

#include <unison/sim/entity_lifecycle.hpp>

#include <entt/entity/registry.hpp>

namespace arena
{

void Lifetimes::update(unison::sim::Frame& frame, const unison::sim::FrameInputs&)
{
    for (const auto [entity, lifetime] : frame.registry.view<Lifetime>().each())
    {
        if (lifetime.framesLeft > 0U)
        {
            --lifetime.framesLeft;
        }

        if (lifetime.framesLeft == 0U)
        {
            unison::sim::destroyEntity(frame, entity);
        }
    }
}

std::string_view Lifetimes::name() const
{
    return "Lifetimes";
}

}
