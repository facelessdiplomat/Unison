#pragma once

#include <boost/pfr/core.hpp>

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace unison::sim
{

namespace detail
{

template <typename T>
inline constexpr bool kIsStdArray = false;

template <typename T, std::size_t Count>
inline constexpr bool kIsStdArray<std::array<T, Count>> = true;

template <typename T>
[[nodiscard]] constexpr bool isPaddingFree();

template <typename T, std::size_t... Index>
[[nodiscard]] constexpr std::size_t sumOfMemberSizes(std::index_sequence<Index...>)
{
    return (sizeof(boost::pfr::tuple_element_t<Index, T>) + ... + std::size_t{0});
}

template <typename T, std::size_t... Index>
[[nodiscard]] constexpr bool membersArePaddingFree(std::index_sequence<Index...>)
{
    return (isPaddingFree<boost::pfr::tuple_element_t<Index, T>>() && ... && true);
}

template <typename T>
[[nodiscard]] constexpr bool isPaddingFree()
{
    if constexpr (std::is_empty_v<T>)
    {
        return true;
    }
    else if constexpr (std::is_array_v<T>)
    {
        return isPaddingFree<std::remove_extent_t<T>>();
    }
    else if constexpr (kIsStdArray<T>)
    {
        return isPaddingFree<typename T::value_type>();
    }
    else if constexpr (std::is_aggregate_v<T> && !std::is_union_v<T>)
    {
        constexpr std::make_index_sequence<boost::pfr::tuple_size_v<T>> indices{};

        return sumOfMemberSizes<T>(indices) == sizeof(T) && membersArePaddingFree<T>(indices);
    }
    else
    {
        return true;
    }
}

}

/// A type whose object representation is nothing but its members, with no gap the compiler inserted
/// for alignment, checked recursively through aggregate members and array elements. A member that is
/// not an aggregate cannot be taken apart and is taken on trust; an empty type has no state to leak.
template <typename T>
concept PaddingFree = detail::isPaddingFree<T>();

}
