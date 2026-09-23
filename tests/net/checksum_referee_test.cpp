#include <unison/net/checksum_referee.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <optional>

namespace
{

constexpr std::uint8_t kThreePlayers = 0b111U;
constexpr std::uint8_t kTwoPlayers = 0b11U;
constexpr std::uint32_t kFrame = 20;

}

TEST_CASE("players who agree on a frame leave nobody in the minority")
{
    unison::net::ChecksumReferee referee;
    static_cast<void>(referee.record(kFrame, 0, 77, kThreePlayers));
    static_cast<void>(referee.record(kFrame, 1, 77, kThreePlayers));

    const std::optional<std::uint8_t> minority = referee.record(kFrame, 2, 77, kThreePlayers);

    REQUIRE(minority == std::optional<std::uint8_t>{0});
}

TEST_CASE("the player who disagrees with the rest is in the minority")
{
    unison::net::ChecksumReferee referee;
    static_cast<void>(referee.record(kFrame, 0, 77, kThreePlayers));
    static_cast<void>(referee.record(kFrame, 1, 78, kThreePlayers));

    const std::optional<std::uint8_t> minority = referee.record(kFrame, 2, 77, kThreePlayers);

    REQUIRE(minority == std::optional<std::uint8_t>{0b010U});
}

TEST_CASE("with no majority every player is in the minority")
{
    unison::net::ChecksumReferee referee;
    static_cast<void>(referee.record(kFrame, 0, 77, kTwoPlayers));

    const std::optional<std::uint8_t> minority = referee.record(kFrame, 1, 78, kTwoPlayers);

    REQUIRE(minority == std::optional<std::uint8_t>{0b11U});
}

TEST_CASE("a frame is judged only once every player has reported it")
{
    unison::net::ChecksumReferee referee;

    const std::optional<std::uint8_t> early = referee.record(kFrame, 0, 77, kThreePlayers);
    const std::optional<std::uint8_t> stillEarly = referee.record(kFrame, 1, 78, kThreePlayers);

    REQUIRE_FALSE(early.has_value());
    REQUIRE_FALSE(stillEarly.has_value());
}

TEST_CASE("a second report from a player for the same frame is ignored")
{
    unison::net::ChecksumReferee referee;
    static_cast<void>(referee.record(kFrame, 0, 77, kTwoPlayers));

    const std::optional<std::uint8_t> repeated = referee.record(kFrame, 0, 78, kTwoPlayers);
    const std::optional<std::uint8_t> judged = referee.record(kFrame, 1, 77, kTwoPlayers);

    REQUIRE_FALSE(repeated.has_value());
    REQUIRE(judged == std::optional<std::uint8_t>{0});
}

TEST_CASE("frames are judged apart from one another")
{
    unison::net::ChecksumReferee referee;
    static_cast<void>(referee.record(kFrame, 0, 77, kTwoPlayers));
    static_cast<void>(referee.record(kFrame + 20, 0, 99, kTwoPlayers));

    const std::optional<std::uint8_t> minority = referee.record(kFrame, 1, 77, kTwoPlayers);

    REQUIRE(minority == std::optional<std::uint8_t>{0});
}
