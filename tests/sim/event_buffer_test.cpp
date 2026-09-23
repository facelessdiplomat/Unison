#include <unison/sim/event_buffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstdint>

namespace
{

struct Scored
{
    std::uint32_t slot = 0;
    std::uint32_t points = 0;
};

struct Died
{
    std::uint32_t slot = 0;
};

constexpr std::uint32_t kFrame = 7;

}

UNISON_EVENT(Scored, unison::sim::EventKind::Predicted);
UNISON_EVENT(Died, unison::sim::EventKind::VerifiedOnly);

TEST_CASE("a raised event keeps its payload")
{
    unison::sim::EventBuffer buffer;

    buffer.raise(kFrame, Scored{2, 50});

    REQUIRE(buffer.size() == 1U);
    REQUIRE(buffer.payloadAt<Scored>(0).slot == 2U);
    REQUIRE(buffer.payloadAt<Scored>(0).points == 50U);
}

TEST_CASE("a raised event is keyed by the frame it was raised in")
{
    unison::sim::EventBuffer buffer;

    const unison::sim::EventKey key = buffer.raise(kFrame, Died{1});

    REQUIRE(key.frame == kFrame);
    REQUIRE(key == buffer.keyAt(0));
}

TEST_CASE("ordinals rise one by one within a type")
{
    unison::sim::EventBuffer buffer;

    REQUIRE(buffer.raise(kFrame, Scored{1, 10}).ordinal == 0U);
    REQUIRE(buffer.raise(kFrame, Scored{2, 20}).ordinal == 1U);
    REQUIRE(buffer.raise(kFrame, Scored{3, 30}).ordinal == 2U);
}

TEST_CASE("each type counts its ordinals apart from the others")
{
    unison::sim::EventBuffer buffer;

    buffer.raise(kFrame, Scored{1, 10});

    REQUIRE(buffer.raise(kFrame, Died{1}).ordinal == 0U);
    REQUIRE(buffer.raise(kFrame, Scored{2, 20}).ordinal == 1U);
}

TEST_CASE("different event types are told apart by their key")
{
    unison::sim::EventBuffer buffer;

    const unison::sim::EventKey scored = buffer.raise(kFrame, Scored{1, 10});
    const unison::sim::EventKey died = buffer.raise(kFrame, Died{1});

    REQUIRE(scored.typeId != died.typeId);
}

TEST_CASE("an event carries whether the view may show it before the frame is verified")
{
    unison::sim::EventBuffer buffer;

    buffer.raise(kFrame, Scored{1, 10});
    buffer.raise(kFrame, Died{1});

    REQUIRE(buffer.kindAt(0) == unison::sim::EventKind::Predicted);
    REQUIRE(buffer.kindAt(1) == unison::sim::EventKind::VerifiedOnly);
}

TEST_CASE("the buffer is empty once it is cleared")
{
    unison::sim::EventBuffer buffer;

    buffer.raise(kFrame, Scored{1, 10});
    buffer.clear();

    REQUIRE(buffer.size() == 0U);
}

TEST_CASE("ordinals start again after the buffer is cleared")
{
    unison::sim::EventBuffer buffer;

    buffer.raise(kFrame, Scored{1, 10});
    buffer.clear();

    REQUIRE(buffer.raise(kFrame + 1, Scored{1, 10}).ordinal == 0U);
}

TEST_CASE("an event appended from another buffer keeps its key, its kind and its payload")
{
    unison::sim::EventBuffer source;
    source.raise(kFrame, Died{3});
    const unison::sim::EventKey scored = source.raise(kFrame, Scored{2, 50});
    unison::sim::EventBuffer target;

    target.append(source, 1);

    REQUIRE(target.size() == 1U);
    REQUIRE(target.keyAt(0) == scored);
    REQUIRE(target.kindAt(0) == unison::sim::EventKind::Predicted);
    REQUIRE(target.payloadAt<Scored>(0).points == 50U);
}

TEST_CASE("appending an event from the buffer itself breaks a contract")
{
    const unison::test::FatalHandlerProbe probe;
    unison::sim::EventBuffer buffer;
    buffer.raise(kFrame, Died{3});

    buffer.append(buffer, 0);

    REQUIRE(probe.failureCount() == 1U);
    REQUIRE(buffer.size() == 1U);
}
