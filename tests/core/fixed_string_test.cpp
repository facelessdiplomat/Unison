#include <unison/core/fixed_string.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace
{

using NameString = unison::FixedString<8>;

}

TEST_CASE("fixed string is empty by default")
{
    const NameString name{};

    REQUIRE(name.size() == 0U);
    REQUIRE(name.view().empty());
}

TEST_CASE("fixed string holds the characters it was built from")
{
    const NameString name{std::string_view{"abc"}};

    REQUIRE(name.size() == 3U);
    REQUIRE(name.view() == "abc");
}

TEST_CASE("fixed string holds content filling its whole capacity")
{
    const NameString name{std::string_view{"abcdefgh"}};

    REQUIRE(name.size() == NameString::kCapacity);
    REQUIRE(name.view() == "abcdefgh");
}

TEST_CASE("fixed strings compare by content")
{
    const NameString name{std::string_view{"abc"}};
    const NameString same{std::string_view{"abc"}};
    const NameString other{std::string_view{"abd"}};

    REQUIRE(name == same);
    REQUIRE(name != other);
}

TEST_CASE("fixed strings with equal content are byte-identical")
{
    const char noisySource[] = "abcdefgh";

    const NameString fromNoisySource{std::string_view{noisySource, 3}};
    const NameString fromExactSource{std::string_view{"abc"}};

    REQUIRE(std::memcmp(&fromNoisySource, &fromExactSource, sizeof(NameString)) == 0);
}

TEST_CASE("fixed string is trivially copyable and free of padding")
{
    STATIC_REQUIRE(std::is_trivially_copyable_v<NameString>);
    STATIC_REQUIRE(sizeof(NameString) == NameString::kCapacity);
}

TEST_CASE("fixed string is built and read at compile time")
{
    constexpr NameString name{std::string_view{"abc"}};

    STATIC_REQUIRE(name.size() == 3U);
    STATIC_REQUIRE(name.view() == "abc");
}
