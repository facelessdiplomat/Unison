#include <catch2/catch_test_macros.hpp>

#include <enet/enet.h>

TEST_CASE("enet initialises and shuts down")
{
    REQUIRE(enet_initialize() == 0);

    enet_deinitialize();
}
