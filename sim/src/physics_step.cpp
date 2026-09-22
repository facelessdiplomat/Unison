#include <unison/sim/physics_step.hpp>

#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

namespace unison::sim
{

void PhysicsStep::update(Frame& frame, const FrameInputs&)
{
    frame.physics.step(frame.dt);

    for (const auto [entity, body, transform] : frame.registry.view<const PhysicsBody, Transform>().each())
    {
        transform = frame.physics.transformOf(body.id);
    }
}

std::string_view PhysicsStep::name() const
{
    return "PhysicsStep";
}

}
