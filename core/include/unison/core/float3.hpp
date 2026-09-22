#pragma once

namespace unison
{

/// Three floats at natural alignment: what a component stores for a position, a direction or a
/// scale. It carries no padding, so its bytes reach a checksum unchanged, while Jolt's SIMD vectors
/// stay inside computation and are converted at the edge.
struct Float3
{
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

}
