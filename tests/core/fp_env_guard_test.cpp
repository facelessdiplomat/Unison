#include <unison/core/fp_env_guard.hpp>

#include <catch2/catch_test_macros.hpp>

#include <xmmintrin.h>

#include <cstdint>
#include <limits>

namespace
{

constexpr std::uint32_t kFlushToZero = 0x8000U;
constexpr std::uint32_t kRoundingControlMask = 0x6000U;
constexpr std::uint32_t kRoundTowardZero = 0x6000U;

class ScopedHostFpState
{
public:
    explicit ScopedHostFpState(std::uint32_t controlWord) : savedControlWord{_mm_getcsr()}
    {
        _mm_setcsr(controlWord);
    }

    ~ScopedHostFpState()
    {
        _mm_setcsr(savedControlWord);
    }

    ScopedHostFpState(const ScopedHostFpState&) = delete;
    ScopedHostFpState& operator=(const ScopedHostFpState&) = delete;
    ScopedHostFpState(ScopedHostFpState&&) = delete;
    ScopedHostFpState& operator=(ScopedHostFpState&&) = delete;

private:
    std::uint32_t savedControlWord;
};

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

TEST_CASE("fp env guard keeps denormals alive while the host flushes them to zero")
{
    const ScopedHostFpState hostState{_mm_getcsr() | kFlushToZero};

    REQUIRE(underflowingProduct() == 0.0F);

    {
        const unison::FpEnvGuard guard;

        REQUIRE(underflowingProduct() != 0.0F);
    }

    REQUIRE(underflowingProduct() == 0.0F);
}

TEST_CASE("fp env guard rounds to nearest while the host rounds toward zero")
{
    const ScopedHostFpState hostState{(_mm_getcsr() & ~kRoundingControlMask) | kRoundTowardZero};

    const float truncated = oneThird();

    {
        const unison::FpEnvGuard guard;

        REQUIRE(oneThird() > truncated);
    }

    REQUIRE(oneThird() == truncated);
}

TEST_CASE("fp env guard restores the host control word on scope exit")
{
    const ScopedHostFpState hostState{_mm_getcsr() | kFlushToZero};
    const std::uint32_t hostControlWord = _mm_getcsr();

    {
        const unison::FpEnvGuard guard;

        REQUIRE(_mm_getcsr() != hostControlWord);
    }

    REQUIRE(_mm_getcsr() == hostControlWord);
}
