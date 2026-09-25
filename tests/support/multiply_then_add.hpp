#pragma once

namespace unison::test
{

/// A multiply followed by an add, compiled in a library of its own under the determinism flags, so a test
/// sees how the deterministic libraries round rather than how the test executable does.
[[nodiscard]] float multiplyThenAdd(float factor, float otherFactor, float addend);

/// The double-precision twin of multiplyThenAdd, compiled the same way.
[[nodiscard]] double multiplyThenAdd(double factor, double otherFactor, double addend);

}
