#pragma once

#include <xmmintrin.h>

#include <cstdint>

namespace unison
{

/// Holds the SSE control word at the architectural default for the length of a scope: round to
/// nearest even, denormals kept, every exception masked, whatever the host had set on this thread.
/// The host's word is restored on exit, so a tick is unaffected by audio or engine code around it.
class [[nodiscard]] FpEnvGuard
{
public:
    FpEnvGuard() : hostControlWord{_mm_getcsr()}
    {
        _mm_setcsr(kDeterministicControlWord);
    }

    ~FpEnvGuard()
    {
        _mm_setcsr(hostControlWord);
    }

    FpEnvGuard(const FpEnvGuard&) = delete;
    FpEnvGuard& operator=(const FpEnvGuard&) = delete;
    FpEnvGuard(FpEnvGuard&&) = delete;
    FpEnvGuard& operator=(FpEnvGuard&&) = delete;

private:
    static constexpr std::uint32_t kDeterministicControlWord = 0x1F80U;

    std::uint32_t hostControlWord;
};

}
