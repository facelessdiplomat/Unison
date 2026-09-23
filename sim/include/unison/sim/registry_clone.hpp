#pragma once

#include <entt/entity/registry.hpp>

namespace unison::sim
{

/// Makes a registry indistinguishable from another: the same entities with the same versions, the same
/// free list, the same packed order in every pool, so the same identifiers from the next create on.
/// What the destination held is dropped, but its storages keep their room, so reusing one allocates less.
void cloneRegistry(const entt::registry& source, entt::registry& destination);

}
