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

TEST_CASE("a spectator that may play no frame yet drops the tick of its host frame")
{
    REQUIRE(unison::session::spectatorTickCorrection(10, 2, 8) == -1);
}

TEST_CASE("a spectator that may play one frame plays its host frame's one tick")
{
    REQUIRE(unison::session::spectatorTickCorrection(10, 2, 7) == 0);
}

TEST_CASE("a spectator that may play many frames adds a tick for every one after the first, seven at most")
{
    REQUIRE(unison::session::spectatorTickCorrection(10, 2, 4) == 3);
    REQUIRE(unison::session::spectatorTickCorrection(40, 0, 0) == unison::session::kCatchUpExtraTicks);
}

TEST_CASE("a spectator further ahead than its delay allows drops the tick of its host frame")
{
    REQUIRE(unison::session::spectatorTickCorrection(1, 3, 0) == -1);
}
