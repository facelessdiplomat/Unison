#include <unison/console/arena_controls.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace
{

constexpr std::uint64_t kHalfASecond = 500'000;
constexpr std::uint64_t kOneTick = 16'667;

}

TEST_CASE("no key held is the neutral input")
{
    unison::console::ArenaControls controls;

    const arena::ArenaInput input = controls.inputFor(unison::console::HeldKeys{}, kOneTick);

    REQUIRE(input.moveX == 0);
    REQUIRE(input.moveY == 0);
    REQUIRE(input.yaw == 0);
    REQUIRE(input.buttons == 0U);
}

TEST_CASE("W and S move forward and back at a full axis, and together not at all")
{
    unison::console::ArenaControls controls;
    unison::console::HeldKeys keys;

    keys.forward = true;
    const std::int8_t forward = controls.inputFor(keys, kOneTick).moveY;
    keys = unison::console::HeldKeys{};
    keys.back = true;
    const std::int8_t back = controls.inputFor(keys, kOneTick).moveY;
    keys.forward = true;
    const std::int8_t both = controls.inputFor(keys, kOneTick).moveY;

    REQUIRE(forward == 127);
    REQUIRE(back == -127);
    REQUIRE(both == 0);
}

TEST_CASE("A and D move left and right at a full axis")
{
    unison::console::ArenaControls controls;
    unison::console::HeldKeys keys;

    keys.left = true;
    const std::int8_t left = controls.inputFor(keys, kOneTick).moveX;
    keys = unison::console::HeldKeys{};
    keys.right = true;
    const std::int8_t right = controls.inputFor(keys, kOneTick).moveX;

    REQUIRE(left == -127);
    REQUIRE(right == 127);
}

TEST_CASE("Space jumps and F fires")
{
    unison::console::ArenaControls controls;
    unison::console::HeldKeys keys;
    keys.jump = true;
    keys.fire = true;

    const arena::ArenaInput input = controls.inputFor(keys, kOneTick);

    REQUIRE(arena::isHeld(input, arena::Button::Jump));
    REQUIRE(arena::isHeld(input, arena::Button::Fire));
}

TEST_CASE("Q turns the aim left and E right, half a turn a second")
{
    unison::console::ArenaControls turningLeft;
    unison::console::ArenaControls turningRight;
    unison::console::HeldKeys left;
    unison::console::HeldKeys right;
    left.turnLeft = true;
    right.turnRight = true;

    const std::int16_t leftYaw = turningLeft.inputFor(left, kHalfASecond).yaw;
    const std::int16_t rightYaw = turningRight.inputFor(right, kHalfASecond).yaw;

    REQUIRE(leftYaw == 16384);
    REQUIRE(rightYaw == -16384);
}

TEST_CASE("the aim turns by the time the key was held, however often it is asked")
{
    unison::console::ArenaControls inSteps;
    unison::console::ArenaControls atOnce;
    unison::console::HeldKeys keys;
    keys.turnLeft = true;

    for (std::uint32_t step = 0; step < 1'000; ++step)
    {
        static_cast<void>(inSteps.inputFor(keys, kHalfASecond / 1'000));
    }

    REQUIRE(inSteps.inputFor(keys, 0).yaw == atOnce.inputFor(keys, kHalfASecond).yaw);
}

TEST_CASE("the aim stays where it was turned once the keys are let go")
{
    unison::console::ArenaControls controls;
    unison::console::HeldKeys keys;
    keys.turnLeft = true;
    const std::int16_t turned = controls.inputFor(keys, kHalfASecond).yaw;

    const std::int16_t later = controls.inputFor(unison::console::HeldKeys{}, kHalfASecond).yaw;

    REQUIRE(later == turned);
}

TEST_CASE("the game keys are noted going down and up by their Windows key codes, and other keys change nothing")
{
    unison::console::HeldKeys keys;

    unison::console::noteKey(keys, 'W', true);
    unison::console::noteKey(keys, 'A', true);
    unison::console::noteKey(keys, 'S', true);
    unison::console::noteKey(keys, 'D', true);
    unison::console::noteKey(keys, ' ', true);
    unison::console::noteKey(keys, 'F', true);
    unison::console::noteKey(keys, 'Q', true);
    unison::console::noteKey(keys, 'E', true);
    unison::console::noteKey(keys, 'Z', true);
    unison::console::noteKey(keys, 'A', false);

    REQUIRE(keys.forward);
    REQUIRE_FALSE(keys.left);
    REQUIRE(keys.back);
    REQUIRE(keys.right);
    REQUIRE(keys.jump);
    REQUIRE(keys.fire);
    REQUIRE(keys.turnLeft);
    REQUIRE(keys.turnRight);
}
