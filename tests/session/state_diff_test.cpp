#include <unison/session/state_diff.hpp>

#include <support/test_components.hpp>
#include <unison/core/error.hpp>
#include <unison/core/rng.hpp>
#include <unison/session/snapshot_serializer.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace
{

constexpr std::uint32_t kSecondEntity = 1;
constexpr std::size_t kOffsetOfPositionY = 4;
const float kNextAfterTwo = std::nextafter(2.0F, 3.0F);

using Difference = tl::expected<std::optional<unison::session::StateDifference>, unison::Error>;

struct Scene
{
    std::uint64_t seed = 42;
    float secondY = 2.0F;
    bool isSecondOnThirdEntity = false;
};

std::vector<std::byte> serializedScene(const Scene& scene)
{
    unison::sim::Frame frame;
    frame.globals.rng = unison::Rng{scene.seed};
    const entt::entity first = frame.registry.create();
    const entt::entity second = frame.registry.create();
    const entt::entity third = frame.registry.create();
    frame.registry.emplace<unison::test::Position>(first, 1.0F, 1.0F);
    frame.registry.emplace<unison::test::Position>(scene.isSecondOnThirdEntity ? third : second, 2.0F, scene.secondY);
    unison::sim::FrameSnapshot snapshot;
    unison::sim::takeSnapshot(frame, snapshot);
    std::vector<std::byte> bytes;
    unison::session::serializeSnapshot(snapshot, bytes);

    return bytes;
}

}

TEST_CASE("two snapshots of one state do not differ")
{
    const Difference difference = unison::session::firstDifferenceOf(serializedScene({}), serializedScene({}));

    REQUIRE(difference.has_value());
    REQUIRE_FALSE(difference->has_value());
}

TEST_CASE("a single field changed is located at its component, entity and byte")
{
    const Difference difference =
        unison::session::firstDifferenceOf(serializedScene({}), serializedScene(Scene{.secondY = kNextAfterTwo}));

    REQUIRE(difference.has_value());
    REQUIRE(difference->has_value());
    REQUIRE((*difference)->part == "Position");
    REQUIRE((*difference)->entity == kSecondEntity);
    REQUIRE((*difference)->byte == kOffsetOfPositionY);
}

TEST_CASE("a component held by another entity is located at the entity, with no byte")
{
    const Difference difference =
        unison::session::firstDifferenceOf(serializedScene({}), serializedScene(Scene{.isSecondOnThirdEntity = true}));

    REQUIRE(difference.has_value());
    REQUIRE(difference->has_value());
    REQUIRE((*difference)->part == "Position");
    REQUIRE((*difference)->entity == kSecondEntity);
    REQUIRE_FALSE((*difference)->byte.has_value());
}

TEST_CASE("a difference outside the pools is located in its part")
{
    const Difference difference =
        unison::session::firstDifferenceOf(serializedScene({}), serializedScene(Scene{.seed = 43}));

    REQUIRE(difference.has_value());
    REQUIRE(difference->has_value());
    REQUIRE((*difference)->part == "globals");
    REQUIRE_FALSE((*difference)->entity.has_value());
}

TEST_CASE("snapshots either reader refuses are refused")
{
    std::vector<std::byte> broken = serializedScene({});
    broken[0] ^= std::byte{0xFF};

    const Difference difference = unison::session::firstDifferenceOf(serializedScene({}), broken);

    REQUIRE_FALSE(difference.has_value());
    REQUIRE(difference.error().code() == unison::ErrorCode::MalformedSnapshot);
}
