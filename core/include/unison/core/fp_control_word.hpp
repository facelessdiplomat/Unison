#pragma once

#include <cstdint>

#if defined(__aarch64__)
    #include <arm_acle.h>
#elif defined(__x86_64__) || defined(_M_X64)
    #include <xmmintrin.h>
#else
    #error "unison: the floating-point control word is known on x86-64 and arm64 only"
#endif

namespace unison
{

/// The calling thread's floating-point control register: MXCSR on x86-64, FPCR on arm64.
using FpControlWord = std::uint64_t;

#if defined(__aarch64__)

/// FPCR's reset value: round to nearest, denormals kept, no exception trapped.
inline constexpr FpControlWord kDeterministicFpControlWord = 0;

/// FZ: denormal inputs and results become zero.
inline constexpr FpControlWord kFlushToZeroBits = FpControlWord{1} << 24U;

/// RMode, the field that chooses the rounding direction.
inline constexpr FpControlWord kRoundingModeBits = FpControlWord{3} << 22U;

/// RMode set to round toward zero.
inline constexpr FpControlWord kRoundTowardZeroBits = FpControlWord{3} << 22U;

/// Reads FPCR.
[[nodiscard]] inline FpControlWord readFpControlWord()
{
    return __arm_rsr64("fpcr");
}

/// Writes FPCR.
inline void writeFpControlWord(FpControlWord word)
{
    __arm_wsr64("fpcr", word);
}

#else

/// MXCSR's power-on value: round to nearest, denormals kept, every exception masked.
inline constexpr FpControlWord kDeterministicFpControlWord = 0x1F80U;

/// FTZ and DAZ: denormal results and inputs become zero.
inline constexpr FpControlWord kFlushToZeroBits = 0x8040U;

/// RC, the field that chooses the rounding direction.
inline constexpr FpControlWord kRoundingModeBits = 0x6000U;

/// RC set to round toward zero.
inline constexpr FpControlWord kRoundTowardZeroBits = 0x6000U;

/// Reads MXCSR.
[[nodiscard]] inline FpControlWord readFpControlWord()
{
    return _mm_getcsr();
}

/// Writes MXCSR, whose defined bits all fit in its low 32.
inline void writeFpControlWord(FpControlWord word)
{
    _mm_setcsr(static_cast<unsigned int>(word));
}

#endif

}
