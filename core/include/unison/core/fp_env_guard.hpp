#pragma once

#include <unison/core/fp_control_word.hpp>

namespace unison
{

/// Holds the thread's floating-point control register at the architecture's default for the length of a
/// scope: round to nearest even, denormals kept, every exception masked, whatever the host had set on this
/// thread. The host's word is restored on exit, so a tick is unaffected by audio or engine code around it.
class [[nodiscard]] FpEnvGuard
{
public:
    FpEnvGuard() : hostControlWord{readFpControlWord()}
    {
        writeFpControlWord(kDeterministicFpControlWord);
    }

    ~FpEnvGuard()
    {
        writeFpControlWord(hostControlWord);
    }

    FpEnvGuard(const FpEnvGuard&) = delete;
    FpEnvGuard& operator=(const FpEnvGuard&) = delete;
    FpEnvGuard(FpEnvGuard&&) = delete;
    FpEnvGuard& operator=(FpEnvGuard&&) = delete;

private:
    FpControlWord hostControlWord;
};

}
