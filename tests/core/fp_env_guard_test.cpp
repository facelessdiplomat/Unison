#include <unison/core/fp_env_guard.hpp>

#include <unison/core/fp_control_word.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/floating_point_probes.hpp>

namespace
{

class ScopedHostFpState
{
public:
    explicit ScopedHostFpState(unison::FpControlWord controlWord) : savedControlWord{unison::readFpControlWord()}
    {
        unison::writeFpControlWord(controlWord);
    }

    ~ScopedHostFpState()
    {
        unison::writeFpControlWord(savedControlWord);
    }

    ScopedHostFpState(const ScopedHostFpState&) = delete;
    ScopedHostFpState& operator=(const ScopedHostFpState&) = delete;
    ScopedHostFpState(ScopedHostFpState&&) = delete;
    ScopedHostFpState& operator=(ScopedHostFpState&&) = delete;

private:
    unison::FpControlWord savedControlWord;
};

}

TEST_CASE("fp env guard keeps denormals alive while the host flushes them to zero")
{
    const ScopedHostFpState hostState{unison::readFpControlWord() | unison::kFlushToZeroBits};

    REQUIRE(unison::test::underflowingProduct() == 0.0F);

    {
        const unison::FpEnvGuard guard;

        REQUIRE(unison::test::underflowingProduct() != 0.0F);
    }

    REQUIRE(unison::test::underflowingProduct() == 0.0F);
}

TEST_CASE("fp env guard rounds to nearest while the host rounds toward zero")
{
    const ScopedHostFpState hostState{(unison::readFpControlWord() & ~unison::kRoundingModeBits) |
                                      unison::kRoundTowardZeroBits};

    const float truncated = unison::test::oneThird();

    {
        const unison::FpEnvGuard guard;

        REQUIRE(unison::test::oneThird() > truncated);
    }

    REQUIRE(unison::test::oneThird() == truncated);
}

TEST_CASE("fp env guard restores the host control word on scope exit")
{
    const ScopedHostFpState hostState{unison::readFpControlWord() | unison::kFlushToZeroBits};
    const unison::FpControlWord hostControlWord = unison::readFpControlWord();

    {
        const unison::FpEnvGuard guard;

        REQUIRE(unison::readFpControlWord() != hostControlWord);
    }

    REQUIRE(unison::readFpControlWord() == hostControlWord);
}
