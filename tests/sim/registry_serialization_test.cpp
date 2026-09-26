#include <unison/sim/registry_serialization.hpp>

#include <support/test_components.hpp>
#include <unison/core/binary_reader.hpp>
#include <unison/core/binary_writer.hpp>
#include <unison/sim/component_registry.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/frame_checksum.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace
{

constexpr std::size_t kCraftedRoom = 4096;

std::vector<std::byte> bytesOf(const entt::registry& registry)
{
    std::vector<std::byte> bytes(unison::sim::serializedSizeOf(registry));
    unison::BinaryWriter writer{bytes};
    const bool isWritten = unison::sim::writeRegistry(writer, registry);

    return isWritten && writer.remaining() == 0 ? bytes : std::vector<std::byte>{};
}

bool isReadInto(std::span<const std::byte> bytes, entt::registry& registry)
{
    unison::BinaryReader reader{bytes};

    return unison::sim::readRegistry(reader, registry) && reader.remaining() == 0;
}

void populate(unison::sim::Frame& frame)
{
    const entt::entity first = frame.registry.create();
    const entt::entity second = frame.registry.create();
    const entt::entity third = frame.registry.create();
    frame.registry.emplace<unison::test::Health>(first, 10);
    frame.registry.emplace<unison::test::Position>(second, 1.0F, 2.0F);
    frame.registry.emplace<unison::test::Health>(third, 30);
    frame.registry.emplace<unison::test::Position>(third, 3.0F, 4.0F);
    frame.registry.destroy(second);
}

std::vector<std::byte> craftedRegistry(std::span<const std::uint32_t> identifiers,
                                       std::uint32_t inUse,
                                       std::span<const std::uint32_t> healthOwners)
{
    std::vector<std::byte> room(kCraftedRoom);
    unison::BinaryWriter writer{room};
    bool isWritten = writer.writeValue(static_cast<std::uint32_t>(identifiers.size())) && writer.writeValue(inUse);

    for (const std::uint32_t identifier : identifiers)
    {
        isWritten = isWritten && writer.writeValue(identifier);
    }

    for (const unison::sim::ComponentInfo& component : unison::sim::componentRegistry().components())
    {
        const bool isHealth = component.name == "Health";
        isWritten = isWritten && writer.writeValue(static_cast<std::uint32_t>(isHealth ? healthOwners.size() : 0U));

        for (const std::uint32_t owner : isHealth ? healthOwners : std::span<const std::uint32_t>{})
        {
            isWritten = isWritten && writer.writeValue(owner) && writer.writeValue(unison::test::Health{5});
        }
    }

    room.resize(isWritten ? writer.size() : 0U);

    return room;
}

}

TEST_CASE("a registry read back from its bytes checksums as the one written")
{
    unison::sim::Frame written;
    populate(written);
    unison::sim::Frame read;

    const bool isRead = isReadInto(bytesOf(written.registry), read.registry);

    REQUIRE(isRead);
    REQUIRE(unison::sim::checksumOf(read) == unison::sim::checksumOf(written));
}

TEST_CASE("a registry's bytes do not depend on the order its pools were first touched in")
{
    unison::sim::Frame healthFirst;
    static_cast<void>(healthFirst.registry.storage<unison::test::Health>());
    static_cast<void>(healthFirst.registry.storage<unison::test::Position>());
    populate(healthFirst);
    unison::sim::Frame positionFirst;
    static_cast<void>(positionFirst.registry.storage<unison::test::Position>());
    static_cast<void>(positionFirst.registry.storage<unison::test::Health>());
    populate(positionFirst);

    REQUIRE(bytesOf(healthFirst.registry) == bytesOf(positionFirst.registry));
}

TEST_CASE("a registry naming an identifier twice is refused")
{
    const std::vector<std::uint32_t> identifiers{0U, 0U};
    unison::sim::Frame read;

    REQUIRE_FALSE(isReadInto(craftedRegistry(identifiers, 2U, {}), read.registry));
}

TEST_CASE("a registry keeping more entities alive than it holds is refused")
{
    const std::vector<std::uint32_t> identifiers{0U};
    unison::sim::Frame read;

    REQUIRE_FALSE(isReadInto(craftedRegistry(identifiers, 2U, {}), read.registry));
}

TEST_CASE("a component on an entity that is not alive is refused")
{
    const std::vector<std::uint32_t> identifiers{0U, 1U};
    const std::vector<std::uint32_t> healthOwners{1U};
    unison::sim::Frame read;

    REQUIRE_FALSE(isReadInto(craftedRegistry(identifiers, 1U, healthOwners), read.registry));
}

TEST_CASE("a component written twice on one entity is refused")
{
    const std::vector<std::uint32_t> identifiers{0U};
    const std::vector<std::uint32_t> healthOwners{0U, 0U};
    unison::sim::Frame read;

    REQUIRE_FALSE(isReadInto(craftedRegistry(identifiers, 1U, healthOwners), read.registry));
}

TEST_CASE("a registry cut short anywhere is refused")
{
    unison::sim::Frame written;
    populate(written);
    const std::vector<std::byte> bytes = bytesOf(written.registry);
    std::size_t refused = 0;

    for (std::size_t length = 0; length < bytes.size(); ++length)
    {
        unison::sim::Frame read;
        refused += isReadInto(std::span{bytes}.first(length), read.registry) ? 0U : 1U;
    }

    REQUIRE(refused == bytes.size());
}
