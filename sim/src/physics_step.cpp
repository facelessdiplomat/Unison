#include <unison/sim/physics_step.hpp>

#include <unison/sim/character.hpp>
#include <unison/sim/physics_body.hpp>
#include <unison/sim/transform.hpp>

namespace unison::sim
{

namespace
{

void moveCharacters(Frame& frame)
{
    const Float3 gravity = frame.physics.gravity();

    for (const auto [entity, character] : frame.registry.view<const CharacterController>().each())
    {
        frame.physics.characters().move(character.id, character.velocity, gravity, frame.dt);
    }
}

void readBodiesBack(Frame& frame)
{
    for (const auto [entity, body, transform] : frame.registry.view<const PhysicsBody, Transform>().each())
    {
        transform = frame.physics.transformOf(body.id);
    }
}

void readCharactersBack(Frame& frame)
{
    for (const auto [entity, character, transform] : frame.registry.view<CharacterController, Transform>().each())
    {
        transform = frame.physics.characters().transformOf(character.id);
        character.velocity = frame.physics.characters().velocityOf(character.id);
        character.ground = frame.physics.characters().groundOf(character.id);
    }
}

}

void PhysicsStep::update(Frame& frame, const FrameInputs&)
{
    moveCharacters(frame);

    frame.physics.step(frame.dt);

    readBodiesBack(frame);
    readCharactersBack(frame);
}

std::string_view PhysicsStep::name() const
{
    return "PhysicsStep";
}

}
