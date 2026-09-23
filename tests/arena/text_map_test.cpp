#include <arena/text_map.hpp>

#include <arena/arena_simulation.hpp>
#include <arena/components.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace
{

std::vector<std::string> linesOf(const std::string& text)
{
    std::istringstream stream{text};
    std::vector<std::string> lines;

    for (std::string line; std::getline(stream, line);)
    {
        lines.push_back(line);
    }

    return lines;
}

}

TEST_CASE("the map of a match is a grid as wide and as deep as the arena, with a line for every player below it")
{
    const arena::ArenaSimulation match{2};

    const std::vector<std::string> lines = linesOf(arena::textMapOf(match.frame()));

    REQUIRE(lines.size() == arena::kMapRows + 2U);

    for (std::size_t row = 0; row < arena::kMapRows; ++row)
    {
        CAPTURE(row);

        REQUIRE(lines[row].size() == arena::kMapColumns);
    }
}

TEST_CASE("walls fence the map in")
{
    const arena::ArenaSimulation match{2};

    const std::vector<std::string> lines = linesOf(arena::textMapOf(match.frame()));

    REQUIRE(lines.front() == std::string(arena::kMapColumns, '#'));
    REQUIRE(lines[arena::kMapRows - 1U] == std::string(arena::kMapColumns, '#'));
    REQUIRE(lines[arena::kMapRows / 2U].front() == '#');
    REQUIRE(lines[arena::kMapRows / 2U].back() == '#');
}

TEST_CASE("a player stands on the map by the number of their slot, with an arrow the way they face")
{
    const auto [yaw, arrowRow, arrowColumn, arrow] =
        GENERATE(table<float, std::size_t, std::size_t, char>({{0.0F, 13U, 24U, 'v'},
                                                               {1.5707964F, 12U, 25U, '>'},
                                                               {3.1415927F, 11U, 24U, '^'},
                                                               {-1.5707964F, 12U, 23U, '<'}}));
    arena::ArenaSimulation match{1};
    auto& registry = match.frame().registry;

    for (const auto [entity, transform, state, slot] :
         registry.view<unison::sim::Transform, arena::CharacterState, const arena::PlayerSlot>().each())
    {
        transform.position = unison::Float3{0.25F, 1.0F, 0.5F};
        state.yaw = yaw;
    }

    const std::vector<std::string> lines = linesOf(arena::textMapOf(match.frame()));

    REQUIRE(lines[12][24] == '0');
    REQUIRE(lines[arrowRow][arrowColumn] == arrow);
}

TEST_CASE("a crate shows where it stands")
{
    const arena::ArenaSimulation match{2};

    const std::vector<std::string> lines = linesOf(arena::textMapOf(match.frame()));

    REQUIRE(lines[12][20] == 'C');
}

TEST_CASE("a shot in flight shows where it is")
{
    arena::ArenaSimulation match{2};
    auto& registry = match.frame().registry;
    const entt::entity shot = registry.create();
    registry.emplace<unison::sim::Transform>(shot, unison::Float3{-9.25F, 1.0F, -6.5F});
    registry.emplace<arena::Projectile>(shot);

    const std::vector<std::string> lines = linesOf(arena::textMapOf(match.frame()));

    REQUIRE(lines[5][5] == '*');
}

TEST_CASE("the line of a player gives their health and kills")
{
    const arena::ArenaSimulation match{2};

    const std::vector<std::string> lines = linesOf(arena::textMapOf(match.frame()));

    REQUIRE(lines[arena::kMapRows] == "slot 0: health 100, kills 0");
    REQUIRE(lines[arena::kMapRows + 1U] == "slot 1: health 100, kills 0");
}

TEST_CASE("the map of a new match of two looks as it did when it was recorded")
{
    const arena::ArenaSimulation match{2};

    REQUIRE(arena::textMapOf(match.frame()) == std::string{R"(################################################
#..............................................#
#..............................................#
#.......................................^......#
#.......0>..............................1......#
#..............................................#
#..............................................#
#...........................===========........#
#...........................============.......#
#.......................CC..============.......#
#............................===========.......#
#..................CC..........................#
#..................CC......CC..................#
#........===========.......CC..................#
#.......============...........................#
#.......============...........................#
#.......===========............................#
#..............................................#
#..............................................#
#..............................................#
#..............................................#
#..............................................#
#..............................................#
################################################
slot 0: health 100, kills 0
slot 1: health 100, kills 0
)"});
}

TEST_CASE("a player waiting to come back is off the map and their line says so")
{
    arena::ArenaSimulation match{1};
    auto& registry = match.frame().registry;

    for (const auto [entity, slot] : registry.view<const arena::PlayerSlot>().each())
    {
        registry.emplace<arena::RespawnTimer>(entity, 60U);
    }

    const std::string map = arena::textMapOf(match.frame());

    REQUIRE(map.find("slot 0: waiting to come back, kills 0") != std::string::npos);
    REQUIRE(map.substr(0, arena::kMapRows * (arena::kMapColumns + 1U)).find('0') == std::string::npos);
}
