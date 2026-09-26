#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace unison::session
{

struct SnapshotPart
{
    std::string_view name;
    std::size_t begin = 0;
    std::size_t end = 0;
    std::size_t elementSize = 0;
};

[[nodiscard]] std::vector<SnapshotPart> partsOf(std::span<const std::byte> bytes);

}
