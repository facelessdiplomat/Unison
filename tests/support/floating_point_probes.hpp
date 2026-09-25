#pragma once

namespace unison::test
{

/// The smallest normal float halved, computed at run time in a translation unit of its own: a denormal,
/// or zero where the calling thread flushes denormals to zero.
[[nodiscard]] float underflowingProduct();

/// One divided by three, computed at run time in a translation unit of its own, so its last bit shows the
/// calling thread's rounding direction.
[[nodiscard]] float oneThird();

}
