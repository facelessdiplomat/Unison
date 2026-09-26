#pragma once

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace unison::session
{

/// Where two serialised snapshots first differ: the part of the frame, the entity when the part is a component's pool,
/// the offset of the first differing byte within that entity's component or within the part, none when the two pools
/// hold another entity there, and the field that byte falls in when the component names its fields.
struct StateDifference
{
    std::string part;
    std::optional<std::uint32_t> entity;
    std::optional<std::size_t> byte;
    std::optional<std::string> field;
};

/// Locates the first difference between two serialised snapshots, or nothing when they are alike byte for byte;
/// bytes either reader refuses are refused with its error.
[[nodiscard]] tl::expected<std::optional<StateDifference>, Error> firstDifferenceOf(std::span<const std::byte> first,
                                                                                    std::span<const std::byte> second);

}
