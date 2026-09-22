#pragma once

#include <entt/entity/registry.hpp>

namespace unison::sim
{

/// Copies a registry into an empty one so that the two are indistinguishable afterwards: the same
/// entities with the same versions, the same free list, the same packed order in every pool, and
/// therefore the same identifiers from the next create onwards. The destination must be empty.
void cloneRegistry(const entt::registry& source, entt::registry& destination);

}
