#include <support/floating_point_probes.hpp>

#include <limits>

namespace unison::test
{

float underflowingProduct()
{
    volatile float smallest = std::numeric_limits<float>::min();
    volatile float half = 0.5F;

    return smallest * half;
}

float oneThird()
{
    volatile float numerator = 1.0F;
    volatile float denominator = 3.0F;

    return numerator / denominator;
}

}
