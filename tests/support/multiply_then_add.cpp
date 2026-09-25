#include <support/multiply_then_add.hpp>

namespace unison::test
{

float multiplyThenAdd(float factor, float otherFactor, float addend)
{
    return factor * otherFactor + addend;
}

double multiplyThenAdd(double factor, double otherFactor, double addend)
{
    return factor * otherFactor + addend;
}

}
