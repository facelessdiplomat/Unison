#include <unison/console/screen.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("the screen shows the status line above the map")
{
    unison::console::ConsoleStatus status;
    status.name = "ada";

    REQUIRE(unison::console::screenOf(status, "#..#\n#..#\n") == "ada idle\n#..#\n#..#\n");
}

TEST_CASE("a screen is drawn over the last one from the top left corner, clearing what the last one left")
{
    REQUIRE(unison::console::redrawnInPlace("ab\ncd") == "\x1b[Hab\x1b[K\ncd\x1b[K\x1b[J");
}

TEST_CASE("a screen breaks no line after its last, so one as tall as the window does not scroll it")
{
    REQUIRE(unison::console::redrawnInPlace("ab\n") == "\x1b[Hab\x1b[K\x1b[J");
}
