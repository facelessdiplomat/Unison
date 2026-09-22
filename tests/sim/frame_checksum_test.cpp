#include <unison/sim/frame_checksum.hpp>

#include <support/test_components.hpp>

#include <catch2/catch_test_macros.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

entt::entity spawn(unison::sim::Frame& frame, float x, std::int32_t points)
{
    const entt::entity entity = frame.registry.create();

    frame.registry.emplace<unison::test::Position>(entity, x, 0.0F);
    frame.registry.emplace<unison::test::Health>(entity, points);

    return entity;
}

std::byte* paddingOf(unison::sim::Globals& globals)
{
    return reinterpret_cast<std::byte*>(&globals) + offsetof(unison::sim::Globals, matchPhase) +
           sizeof(unison::sim::MatchPhase);
}

}

TEST_CASE("frames holding the same state have the same checksum")
{
    unison::sim::Frame first;
    unison::sim::Frame second;

    spawn(first, 1.0F, 10);
    spawn(second, 1.0F, 10);

    REQUIRE(unison::sim::checksumOf(first) == unison::sim::checksumOf(second));
}

TEST_CASE("changing one component field changes the checksum")
{
    unison::sim::Frame frame;
    const entt::entity entity = spawn(frame, 1.0F, 10);
    const std::uint64_t before = unison::sim::checksumOf(frame);

    frame.registry.get<unison::test::Health>(entity).points = 11;

    REQUIRE(unison::sim::checksumOf(frame) != before);
}

TEST_CASE("changing the globals changes the checksum")
{
    unison::sim::Frame frame;
    const std::uint64_t before = unison::sim::checksumOf(frame);

    frame.globals.matchPhase = unison::sim::MatchPhase::Playing;

    REQUIRE(unison::sim::checksumOf(frame) != before);
}

TEST_CASE("the checksum does not depend on the order the pools were first touched")
{
    unison::sim::Frame inOneOrder;
    unison::sim::Frame inTheOther;

    const entt::entity firstEntity = inOneOrder.registry.create();
    inOneOrder.registry.emplace<unison::test::Position>(firstEntity, 1.0F, 0.0F);
    inOneOrder.registry.emplace<unison::test::Health>(firstEntity, 10);

    const entt::entity secondEntity = inTheOther.registry.create();
    inTheOther.registry.emplace<unison::test::Health>(secondEntity, 10);
    inTheOther.registry.emplace<unison::test::Position>(secondEntity, 1.0F, 0.0F);

    REQUIRE(unison::sim::checksumOf(inOneOrder) == unison::sim::checksumOf(inTheOther));
}

TEST_CASE("the padding inside the globals stays out of the checksum")
{
    STATIC_REQUIRE(sizeof(unison::sim::Globals) >
                   offsetof(unison::sim::Globals, matchPhase) + sizeof(unison::sim::MatchPhase));

    unison::sim::Frame frame;
    const std::uint64_t before = unison::sim::checksumOf(frame);

    *paddingOf(frame.globals) = std::byte{0xFF};

    REQUIRE(unison::sim::checksumOf(frame) == before);
}

TEST_CASE("a destroyed entity leaves its mark on the checksum")
{
    unison::sim::Frame frame;
    const entt::entity entity = spawn(frame, 1.0F, 10);
    spawn(frame, 2.0F, 20);

    const std::uint64_t before = unison::sim::checksumOf(frame);

    frame.registry.destroy(entity);

    REQUIRE(unison::sim::checksumOf(frame) != before);
}
