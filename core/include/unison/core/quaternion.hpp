#pragma once

namespace unison
{

/// Four floats at natural alignment: what a component stores for a rotation, padding-free for the
/// same reason as Float3. It defaults to the identity rotation rather than to all zeroes, because a
/// zero quaternion is not a rotation and would turn into NaN the moment physics normalised it.
struct Quaternion
{
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

}
