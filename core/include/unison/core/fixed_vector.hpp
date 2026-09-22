#pragma once

#include <unison/core/contract.hpp>

#include <array>
#include <cstddef>
#include <type_traits>

namespace unison
{

/// Fixed-capacity vector with inline storage, trivially copyable whenever its element is.
/// Vacated slots are value-initialised, so the byte image depends only on the elements held and
/// stays safe to hash and to snapshot. Indexing, popping or pushing past the bounds breaks the contract.
template <typename T, std::size_t Capacity>
class FixedVector
{
    static_assert(std::is_trivially_copyable_v<T>, "FixedVector elements must be trivially copyable");
    static_assert(Capacity > 0, "FixedVector needs room for at least one element");

public:
    static constexpr std::size_t kCapacity = Capacity;

    constexpr void pushBack(const T& value)
    {
        UNISON_ASSERT(count < Capacity);

        values[count] = value;
        ++count;
    }

    constexpr void popBack()
    {
        UNISON_ASSERT(count > 0);

        --count;
        values[count] = T{};
    }

    constexpr void clear()
    {
        while (count > 0)
        {
            popBack();
        }
    }

    [[nodiscard]] constexpr T& operator[](std::size_t index)
    {
        UNISON_ASSERT(index < count);

        return values[index];
    }

    [[nodiscard]] constexpr const T& operator[](std::size_t index) const
    {
        UNISON_ASSERT(index < count);

        return values[index];
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return count;
    }

    [[nodiscard]] constexpr bool isFull() const
    {
        return count == Capacity;
    }

    [[nodiscard]] constexpr T* begin()
    {
        return values.data();
    }

    [[nodiscard]] constexpr T* end()
    {
        return values.data() + count;
    }

    [[nodiscard]] constexpr const T* begin() const
    {
        return values.data();
    }

    [[nodiscard]] constexpr const T* end() const
    {
        return values.data() + count;
    }

private:
    std::array<T, Capacity> values{};
    std::size_t count = 0;
};

}
