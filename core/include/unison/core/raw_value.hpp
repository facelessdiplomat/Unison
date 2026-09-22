#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace unison
{

namespace detail
{

template <typename T>
inline constexpr bool kIsSpan = false;

template <typename T, std::size_t Extent>
inline constexpr bool kIsSpan<std::span<T, Extent>> = true;

template <typename T>
inline constexpr bool kIsStringView = false;

template <typename Character, typename Traits>
inline constexpr bool kIsStringView<std::basic_string_view<Character, Traits>> = true;

}

/// A value whose object bytes carry its whole meaning, so hashing or serialising those bytes is
/// faithful: trivially copyable, and none of the view types whose bytes are an address that differs
/// between processes. A struct holding a pointer still passes, so components stay free of pointers.
template <typename T>
concept RawValue =
    std::is_trivially_copyable_v<T> && !std::is_pointer_v<T> && !detail::kIsSpan<T> && !detail::kIsStringView<T>;

}
