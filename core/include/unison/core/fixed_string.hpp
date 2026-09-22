#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <string_view>

namespace unison
{

/// Fixed-capacity string with inline storage, trivially copyable and free of padding, holding up to
/// Capacity characters with no terminator stored. Everything past the content is zero, so equal content
/// always has an equal byte image. Text longer than Capacity, or carrying a null, breaks the contract.
template <std::size_t Capacity>
class FixedString
{
    static_assert(Capacity > 0, "FixedString needs room for at least one character");

public:
    static constexpr std::size_t kCapacity = Capacity;

    constexpr FixedString() = default;

    constexpr explicit FixedString(std::string_view text)
    {
        assert(text.size() <= Capacity);
        assert(text.find('\0') == std::string_view::npos);

        for (std::size_t index = 0; index < text.size(); ++index)
        {
            characters[index] = text[index];
        }
    }

    [[nodiscard]] constexpr bool operator==(const FixedString& other) const = default;

    [[nodiscard]] constexpr std::string_view view() const
    {
        return std::string_view{characters.data(), size()};
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        std::size_t length = 0;

        while (length < Capacity && characters[length] != '\0')
        {
            ++length;
        }

        return length;
    }

private:
    std::array<char, Capacity> characters{};
};

}
