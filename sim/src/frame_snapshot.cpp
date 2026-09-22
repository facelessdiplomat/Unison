#include <unison/sim/frame_snapshot.hpp>

#include <unison/sim/physics_body.hpp>
#include <unison/sim/registry_clone.hpp>

namespace unison::sim
{

void takeSnapshot(const Frame& frame, FrameSnapshot& snapshot)
{
    snapshot.frameNumber = frame.frameNumber;
    snapshot.dt = frame.dt;
    snapshot.globals = frame.globals;
    snapshot.registry = entt::registry{};

    cloneRegistry(frame.registry, snapshot.registry);

    frame.physics.saveState(snapshot.physicsState);
}

void restoreSnapshot(const FrameSnapshot& snapshot, Frame& frame)
{
    frame.frameNumber = snapshot.frameNumber;
    frame.dt = snapshot.dt;
    frame.globals = snapshot.globals;
    frame.registry = entt::registry{};

    cloneRegistry(snapshot.registry, frame.registry);

    reconcileBodies(frame);

    frame.physics.restoreState(snapshot.physicsState);
}

}
