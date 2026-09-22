#include <unison/sim/pipeline_hash.hpp>

#include <unison/core/hasher.hpp>

#include <cstddef>
#include <span>
#include <string_view>

namespace unison::sim
{

std::uint64_t hashOf(const SystemPipeline& pipeline)
{
    Hasher hasher;

    hasher.add(static_cast<std::uint32_t>(pipeline.size()));

    for (std::size_t index = 0; index < pipeline.size(); ++index)
    {
        const std::string_view name = pipeline.at(index).name();

        hasher.add(static_cast<std::uint32_t>(name.size()));
        hasher.add(std::as_bytes(std::span{name}));
    }

    return hasher.finish();
}

}
