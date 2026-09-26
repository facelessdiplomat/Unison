#include <unison/session/catch_up.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("a client that has not started catching up asks for no extra tick, however far behind it is")
{
    unison::session::CatchUp catchUp;
    catchUp.hear(40);

    REQUIRE_FALSE(catchUp.isBehind());
    REQUIRE_FALSE(catchUp.extraTicks(5).has_value());
}

TEST_CASE("a client catching up asks for a tick more for every frame it is behind, seven at most")
{
    unison::session::CatchUp catchUp;
    catchUp.hear(40);
    catchUp.start();

    REQUIRE(catchUp.isBehind());
    REQUIRE(catchUp.extraTicks(5) == unison::session::kCatchUpExtraTicks);
    REQUIRE(catchUp.extraTicks(37) == 3);
}

TEST_CASE("a client stops catching up once it has played the newest frame it heard confirmed, for good")
{
    unison::session::CatchUp catchUp;
    catchUp.hear(40);
    catchUp.start();

    catchUp.update(40);
    catchUp.hear(60);
    catchUp.update(41);

    REQUIRE_FALSE(catchUp.isBehind());
    REQUIRE_FALSE(catchUp.extraTicks(41).has_value());
}

TEST_CASE("a client catches up with the newest frame it heard confirmed, not with one heard after it")
{
    unison::session::CatchUp catchUp;
    catchUp.hear(40);
    catchUp.hear(30);
    catchUp.start();

    catchUp.update(35);

    REQUIRE(catchUp.isBehind());
    REQUIRE(catchUp.extraTicks(35) == 5);
}
