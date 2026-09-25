#include <unison/core/fp_control_word.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("the floating-point control word reads back the rounding mode written to it")
{
    const unison::FpControlWord hostWord = unison::readFpControlWord();
    const unison::FpControlWord towardZero = (hostWord & ~unison::kRoundingModeBits) | unison::kRoundTowardZeroBits;

    unison::writeFpControlWord(towardZero);
    const unison::FpControlWord readBack = unison::readFpControlWord();
    unison::writeFpControlWord(hostWord);

    REQUIRE((readBack & unison::kRoundingModeBits) == unison::kRoundTowardZeroBits);
}

TEST_CASE("the floating-point control word reads back the flush-to-zero bits written to it")
{
    const unison::FpControlWord hostWord = unison::readFpControlWord();

    unison::writeFpControlWord(hostWord | unison::kFlushToZeroBits);
    const unison::FpControlWord readBack = unison::readFpControlWord();
    unison::writeFpControlWord(hostWord);

    REQUIRE((readBack & unison::kFlushToZeroBits) == unison::kFlushToZeroBits);
}
