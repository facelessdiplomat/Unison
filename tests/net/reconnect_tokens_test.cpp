#include <unison/net/reconnect_tokens.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <set>

TEST_CASE("a reconnect token is never nought, which stands for no token")
{
    unison::net::ReconnectTokens tokens{0};

    for (int draw = 0; draw < 1000; ++draw)
    {
        REQUIRE(tokens.next() != 0U);
    }
}

TEST_CASE("reconnect tokens drawn one after another all differ")
{
    unison::net::ReconnectTokens tokens{7};
    std::set<std::uint64_t> drawn;

    for (int draw = 0; draw < 1000; ++draw)
    {
        drawn.insert(tokens.next());
    }

    REQUIRE(drawn.size() == 1000U);
}

TEST_CASE("the same seed draws the same reconnect tokens and another seed other ones")
{
    unison::net::ReconnectTokens first{7};
    unison::net::ReconnectTokens again{7};
    unison::net::ReconnectTokens other{8};

    const std::uint64_t drawn = first.next();

    REQUIRE(again.next() == drawn);
    REQUIRE(other.next() != drawn);
}
