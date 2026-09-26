#include <unison/session/state_diff.hpp>

#include "snapshot_parts.hpp"

#include <unison/session/snapshot_serializer.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <algorithm>
#include <cstring>
#include <vector>

namespace unison::session
{

namespace
{

constexpr std::size_t kIdentifierSize = sizeof(std::uint32_t);

StateDifference differenceIn(const SnapshotPart& part, std::span<const std::byte> first, std::size_t offset)
{
    const std::size_t within = offset - part.begin;

    if (part.elementSize == 0 || within < kIdentifierSize)
    {
        return StateDifference{std::string{part.name}, std::nullopt, within};
    }

    const std::size_t elementBegin = kIdentifierSize + (within - kIdentifierSize) / part.elementSize * part.elementSize;
    const std::size_t inElement = within - elementBegin;
    std::uint32_t entity = 0;
    std::memcpy(&entity, first.subspan(part.begin + elementBegin, kIdentifierSize).data(), kIdentifierSize);

    return StateDifference{std::string{part.name},
                           entity,
                           inElement < kIdentifierSize ? std::nullopt : std::optional{inElement - kIdentifierSize}};
}

}

tl::expected<std::optional<StateDifference>, Error> firstDifferenceOf(std::span<const std::byte> first,
                                                                      std::span<const std::byte> second)
{
    sim::FrameSnapshot scratch;

    for (const std::span<const std::byte> bytes : {first, second})
    {
        if (const tl::expected<void, Error> read = deserializeSnapshot(bytes, scratch); !read.has_value())
        {
            return tl::unexpected{read.error()};
        }
    }

    const auto mismatch = std::ranges::mismatch(first, second);

    if (mismatch.in1 == first.end() && mismatch.in2 == second.end())
    {
        return std::nullopt;
    }

    const auto offset =
        static_cast<std::size_t>(std::min(mismatch.in1 - first.begin(), static_cast<std::ptrdiff_t>(first.size()) - 1));
    const std::vector<SnapshotPart> parts = partsOf(first);
    const auto part =
        std::ranges::find_if(parts, [offset](const SnapshotPart& candidate) { return offset < candidate.end; });

    return differenceIn(part == parts.end() ? parts.back() : *part, first, offset);
}

}
