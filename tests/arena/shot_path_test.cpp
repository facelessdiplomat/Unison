#include <arena/shot_path.hpp>

#include <catch2/catch_test_macros.hpp>

namespace
{

constexpr float kShotRadius = 0.12F;
constexpr float kCapsuleRadius = 0.3F;

const unison::Float3 kBottom{4.0F, 0.3F, 0.0F};
const unison::Float3 kTop{4.0F, 1.5F, 0.0F};

arena::ShotApproach sweep(const unison::Float3& from, const unison::Float3& to)
{
    return arena::sweepPastCapsule(from, to, kShotRadius, kBottom, kTop, kCapsuleRadius);
}

}

TEST_CASE("a shot aimed at a body reaches it partway along its path")
{
    const arena::ShotApproach approach = sweep(unison::Float3{0.0F, 1.0F, 0.0F}, unison::Float3{8.0F, 1.0F, 0.0F});

    REQUIRE(approach.touched);
    REQUIRE(approach.fraction > 0.49F);
    REQUIRE(approach.fraction < 0.51F);
}

TEST_CASE("a shot going wide of a body touches nothing")
{
    const arena::ShotApproach approach = sweep(unison::Float3{0.0F, 1.0F, 2.0F}, unison::Float3{8.0F, 1.0F, 2.0F});

    REQUIRE_FALSE(approach.touched);
}

TEST_CASE("a shot passing over a body touches nothing")
{
    const arena::ShotApproach approach = sweep(unison::Float3{0.0F, 3.0F, 0.0F}, unison::Float3{8.0F, 3.0F, 0.0F});

    REQUIRE_FALSE(approach.touched);
}

TEST_CASE("a shot that stops short of a body touches nothing")
{
    const arena::ShotApproach approach = sweep(unison::Float3{0.0F, 1.0F, 0.0F}, unison::Float3{2.0F, 1.0F, 0.0F});

    REQUIRE_FALSE(approach.touched);
}

TEST_CASE("a shot grazing the side of a body still reaches it")
{
    const arena::ShotApproach approach = sweep(unison::Float3{0.0F, 1.0F, 0.39F}, unison::Float3{8.0F, 1.0F, 0.39F});

    REQUIRE(approach.touched);
}

TEST_CASE("a shot that would jump over a body in one tick still reaches it")
{
    const arena::ShotApproach approach = sweep(unison::Float3{3.0F, 1.0F, 0.0F}, unison::Float3{5.0F, 1.0F, 0.0F});

    REQUIRE(approach.touched);
    REQUIRE(approach.fraction > 0.2F);
    REQUIRE(approach.fraction < 0.6F);
}

TEST_CASE("a shot reaching the head of a body reaches it")
{
    const arena::ShotApproach approach = sweep(unison::Float3{0.0F, 1.6F, 0.0F}, unison::Float3{8.0F, 1.6F, 0.0F});

    REQUIRE(approach.touched);
}
