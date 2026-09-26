#pragma once

#include <unison/core/binary_reader.hpp>
#include <unison/core/binary_writer.hpp>

#include <entt/entity/registry.hpp>

#include <cstddef>

namespace unison::sim
{

/// The bytes `writeRegistry` takes for a registry.
[[nodiscard]] std::size_t serializedSizeOf(const entt::registry& registry);

/// Writes a registry as its entity storage, the identifiers in packed order and how many of them are alive, then
/// every registered component pool in registration order. Returns false when the writer runs out of room.
[[nodiscard]] bool writeRegistry(BinaryWriter& writer, const entt::registry& registry);

/// Reads a registry `writeRegistry` wrote into one emptied first. Returns false for bytes that end early, name an
/// identifier twice or one no storage could hold, keep more entities alive than there are, or put a component on
/// an entity that is not alive or two on one entity; the registry is left half read then.
[[nodiscard]] bool readRegistry(BinaryReader& reader, entt::registry& registry);

}
