#include <support/multiply_then_add.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("a multiply followed by an add rounds twice")
{
    const float slightlyAboveOne = 1.0F + 0x1.0p-23F;
    const float negatedRoundedSquare = -(1.0F + 0x1.0p-22F);

    REQUIRE(unison::test::multiplyThenAdd(slightlyAboveOne, slightlyAboveOne, negatedRoundedSquare) == 0.0F);
}

TEST_CASE("a multiply followed by an add rounds twice in double precision")
{
    const double slightlyAboveOne = 1.0 + 0x1.0p-52;
    const double negatedRoundedSquare = -(1.0 + 0x1.0p-51);

    REQUIRE(unison::test::multiplyThenAdd(slightlyAboveOne, slightlyAboveOne, negatedRoundedSquare) == 0.0);
}
