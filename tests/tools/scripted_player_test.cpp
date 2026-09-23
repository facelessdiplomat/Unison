#include <unison/runner/scripted_player.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

namespace
{

constexpr std::uint32_t kFrames = 300;

std::vector<arena::ArenaInput> inputsOf(unison::runner::ScriptedPlayer& player)
{
    std::vector<arena::ArenaInput> inputs;

    for (std::uint32_t frame = 0; frame < kFrames; ++frame)
    {
        inputs.push_back(player.nextInput());
    }

    return inputs;
}

bool isSameInput(const arena::ArenaInput& first, const arena::ArenaInput& second)
{
    return first.moveX == second.moveX && first.moveY == second.moveY && first.yaw == second.yaw &&
           first.buttons == second.buttons;
}

std::uint32_t coursesIn(const std::vector<arena::ArenaInput>& inputs)
{
    std::uint32_t courses = 1;

    for (std::size_t frame = 1; frame < inputs.size(); ++frame)
    {
        const bool turned = inputs[frame].moveX != inputs[frame - 1].moveX ||
                            inputs[frame].moveY != inputs[frame - 1].moveY ||
                            inputs[frame].yaw != inputs[frame - 1].yaw;
        courses += turned ? 1U : 0U;
    }

    return courses;
}

}

TEST_CASE("scripted players of one seed and one number play the same inputs")
{
    unison::runner::ScriptedPlayer first{7, 1};
    unison::runner::ScriptedPlayer again{7, 1};

    const std::vector<arena::ArenaInput> firstInputs = inputsOf(first);
    const std::vector<arena::ArenaInput> againInputs = inputsOf(again);

    for (std::uint32_t frame = 0; frame < kFrames; ++frame)
    {
        REQUIRE(isSameInput(firstInputs[frame], againInputs[frame]));
    }
}

TEST_CASE("two players of one run play inputs of their own")
{
    unison::runner::ScriptedPlayer first{7, 0};
    unison::runner::ScriptedPlayer second{7, 1};

    const std::vector<arena::ArenaInput> firstInputs = inputsOf(first);
    const std::vector<arena::ArenaInput> secondInputs = inputsOf(second);
    std::uint32_t sameFrames = 0;

    for (std::uint32_t frame = 0; frame < kFrames; ++frame)
    {
        sameFrames += isSameInput(firstInputs[frame], secondInputs[frame]) ? 1U : 0U;
    }

    REQUIRE(sameFrames < kFrames / 2U);
}

TEST_CASE("a scripted player keeps a course for a while before it picks another")
{
    unison::runner::ScriptedPlayer player{7, 0};

    const std::uint32_t courses = coursesIn(inputsOf(player));

    REQUIRE(courses >= kFrames / 40U);
    REQUIRE(courses <= kFrames / 10U + 1U);
}

TEST_CASE("a scripted player jumps and fires now and then")
{
    unison::runner::ScriptedPlayer player{7, 0};
    std::uint32_t jumps = 0;
    std::uint32_t shots = 0;

    for (const arena::ArenaInput& input : inputsOf(player))
    {
        jumps += arena::isHeld(input, arena::Button::Jump) ? 1U : 0U;
        shots += arena::isHeld(input, arena::Button::Fire) ? 1U : 0U;
    }

    REQUIRE(jumps > 0U);
    REQUIRE(jumps < kFrames / 4U);
    REQUIRE(shots > 0U);
    REQUIRE(shots < kFrames / 2U);
}
