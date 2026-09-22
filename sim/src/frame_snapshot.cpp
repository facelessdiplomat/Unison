#include <unison/sim/frame_snapshot.hpp>

#include <unison/sim/registry_clone.hpp>

namespace unison::sim
{

void takeSnapshot(const Frame& frame, FrameSnapshot& snapshot)
{
    snapshot.frameNumber = frame.frameNumber;
    snapshot.dt = frame.dt;
    snapshot.globals = frame.globals;
    snapshot.physicsState.clear();
    snapshot.registry = entt::registry{};

    cloneRegistry(frame.registry, snapshot.registry);
}

void restoreSnapshot(const FrameSnapshot& snapshot, Frame& frame)
{
    frame.frameNumber = snapshot.frameNumber;
    frame.dt = snapshot.dt;
    frame.globals = snapshot.globals;
    frame.registry = entt::registry{};

    cloneRegistry(snapshot.registry, frame.registry);
}

}
