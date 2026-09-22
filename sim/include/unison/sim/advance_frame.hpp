#pragma once

#include <unison/sim/frame.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/system_pipeline.hpp>

namespace unison::sim
{

/// Runs one tick: the systems of the pipeline once each, under the deterministic floating-point
/// environment, and then the frame moves on. The events of the previous tick are cleared first, so
/// what a caller finds in the buffer afterwards is exactly what this tick raised.
void advanceFrame(Frame& frame, const SystemPipeline& pipeline, const FrameInputs& inputs);

}
