#include <arena/arena_input.hpp>

#include <catch2/catch_test_macros.hpp>

#include <unison/sim/frame_inputs.hpp>

#include <cstdint>

TEST_CASE("an arena input is small enough and plain enough to travel")
{
    STATIC_REQUIRE(unison::sim::InputTraits<arena::ArenaInput>::isValid);
    STATIC_REQUIRE(sizeof(arena::ArenaInput) <= 8U);
}

TEST_CASE("a button is held only while its bit is set")
{
    arena::ArenaInput input;
    input.buttons = static_cast<std::uint16_t>(arena::Button::Fire);

    REQUIRE(arena::isHeld(input, arena::Button::Fire));
    REQUIRE_FALSE(arena::isHeld(input, arena::Button::Jump));
}

TEST_CASE("a full axis asks for full speed")
{
    REQUIRE(arena::axisOf(127) == 1.0F);
    REQUIRE(arena::axisOf(-127) == -1.0F);
    REQUIRE(arena::axisOf(0) == 0.0F);
}

TEST_CASE("the quantised aim covers a whole turn")
{
    arena::ArenaInput input;

    REQUIRE(arena::yawOf(input) == 0.0F);

    input.yaw = 16384;

    REQUIRE(arena::yawOf(input) > 1.57F);
    REQUIRE(arena::yawOf(input) < 1.58F);
}

TEST_CASE("an input travels through a slot unchanged")
{
    arena::ArenaInput input;
    input.moveX = -12;
    input.moveY = 34;
    input.yaw = -4096;
    input.buttons = static_cast<std::uint16_t>(arena::Button::Jump);

    unison::sim::FrameInputs inputs;
    inputs.set(0U, input, unison::sim::InputFlags::Present);

    const arena::ArenaInput back = inputs.get<arena::ArenaInput>(0U);

    REQUIRE(back.moveX == -12);
    REQUIRE(back.moveY == 34);
    REQUIRE(back.yaw == -4096);
    REQUIRE(arena::isHeld(back, arena::Button::Jump));
}
