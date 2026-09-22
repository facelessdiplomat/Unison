#include <unison/sim/advance_frame.hpp>

#include <unison/core/fp_env_guard.hpp>

namespace unison::sim
{

void advanceFrame(Frame& frame, const SystemPipeline& pipeline, const FrameInputs& inputs)
{
    const FpEnvGuard guard;

    frame.events.clear();
    pipeline.update(frame, inputs);

    ++frame.frameNumber;
}

}
