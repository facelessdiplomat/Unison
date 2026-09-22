#include <unison/core/error.hpp>

#include <catch2/catch_test_macros.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace
{

tl::expected<std::uint32_t, unison::Error> decode(bool succeed)
{
    if (!succeed)
    {
        return tl::unexpected{unison::Error{unison::ErrorCode::TruncatedInput, "the buffer ended early"}};
    }

    return 7U;
}

}

TEST_CASE("an error carries a code and a message")
{
    constexpr unison::Error error{unison::ErrorCode::TruncatedInput, "the buffer ended early"};

    STATIC_REQUIRE(error.code() == unison::ErrorCode::TruncatedInput);
    STATIC_REQUIRE(error.message() == "the buffer ended early");
}

TEST_CASE("an error is small enough to return by value")
{
    STATIC_REQUIRE(std::is_trivially_copyable_v<unison::Error>);
}

TEST_CASE("expected round trips a value")
{
    const auto decoded = decode(true);

    REQUIRE(decoded.has_value());
    REQUIRE(decoded.value() == 7U);
}

TEST_CASE("expected round trips an error")
{
    const auto decoded = decode(false);

    REQUIRE_FALSE(decoded.has_value());
    REQUIRE(decoded.error().code() == unison::ErrorCode::TruncatedInput);
    REQUIRE(decoded.error().message() == "the buffer ended early");
}
