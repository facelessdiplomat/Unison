#pragma once

#include <entt/entity/registry.hpp>

namespace unison::sim
{

inline void emptyStorages(entt::registry& registry)
{
    for (auto&& pool : registry.storage())
    {
        pool.second.clear();
    }

    registry.storage<entt::entity>().clear();
}

}
