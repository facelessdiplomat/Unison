#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/message_codec.hpp>
#include <unison/net/relay_core.hpp>
#include <unison/session/networked_session.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace
{

constexpr std::uint8_t kPlayers = 2;
constexpr std::uint32_t kFrames = 1000;
constexpr std::uint32_t kMostTicks = 1100;
constexpr std::uint64_t kTickMicroseconds = 16'667;

using Reports = std::vector<std::pair<std::uint32_t, std::uint64_t>>;

unison::net::SessionConfig twoPlayerMatch()
{
    unison::net::SessionConfig config;
    config.slotCount = kPlayers;
    config.inputSize = sizeof(arena::ArenaInput);
    config.checksumInterval = 1;

    return config;
}

class ChecksumTap final : public unison::net::IMessageReceiver
{
public:
    ChecksumTap(unison::net::IMessageReceiver& relay, unison::net::PeerId first, unison::net::PeerId second)
        : relay{relay}, first{first}, second{second}
    {
    }

    void receive(unison::net::PeerId from, unison::net::Channel channel, std::span<const std::byte> message) override
    {
        const auto decoded = unison::net::decode(message);

        if (decoded.has_value() && std::holds_alternative<unison::net::Checksum>(*decoded))
        {
            const auto& checksum = std::get<unison::net::Checksum>(*decoded);

            (from == first ? firstReports : secondReports).emplace_back(checksum.frame, checksum.checksum);
        }

        relay.receive(from, channel, message);
    }

    Reports firstReports;
    Reports secondReports;

private:
    unison::net::IMessageReceiver& relay;
    unison::net::PeerId first;
    unison::net::PeerId second;
};

bool hasVerified(const unison::session::NetworkedSession& client, std::uint32_t frames)
{
    return client.session() != nullptr && client.session()->verifiedFrame() >= frames;
}

void play(unison::session::NetworkedSession& client,
          const unison::sim::FrameInputs& scripted,
          std::size_t slot,
          std::uint64_t now)
{
    client.setLocalInput(scripted.bytesAt(slot).first(sizeof(arena::ArenaInput)));
    client.update(now);
    client.tick();
}

}

TEST_CASE("two clients of an arena match played through a relay agree on every one of a thousand frames")
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& relayEnd = hub.join();
    unison::net::LoopbackEndpoint& firstEnd = hub.join();
    unison::net::LoopbackEndpoint& secondEnd = hub.join();
    const unison::net::ManualClock clock;
    unison::net::RelayCore relay{relayEnd, clock, twoPlayerMatch()};
    ChecksumTap tap{relay, firstEnd.id(), secondEnd.id()};
    arena::ArenaSimulation firstMatch{kPlayers};
    arena::ArenaSimulation secondMatch{kPlayers};
    unison::session::NetworkedSession first{
        firstMatch.frame(), firstMatch.pipeline(), twoPlayerMatch(), firstEnd, relayEnd.id()};
    unison::session::NetworkedSession second{
        secondMatch.frame(), secondMatch.pipeline(), twoPlayerMatch(), secondEnd, relayEnd.id()};
    first.join();
    second.join();

    for (std::uint32_t tick = 0; tick < kMostTicks && !(hasVerified(first, kFrames) && hasVerified(second, kFrames));
         ++tick)
    {
        relayEnd.poll(tap);

        const unison::sim::FrameInputs scripted = unison::test::scriptedInputs(tick);
        play(first, scripted, 0, tick * kTickMicroseconds);
        play(second, scripted, 1, tick * kTickMicroseconds);
    }

    relayEnd.poll(tap);

    REQUIRE(hasVerified(first, kFrames));
    REQUIRE(hasVerified(second, kFrames));
    REQUIRE(tap.firstReports.size() >= kFrames);
    REQUIRE(tap.secondReports.size() >= kFrames);
    REQUIRE(Reports(tap.firstReports.begin(), tap.firstReports.begin() + kFrames) ==
            Reports(tap.secondReports.begin(), tap.secondReports.begin() + kFrames));
    REQUIRE(tap.firstReports[kFrames - 1].first == kFrames);
    REQUIRE_FALSE(first.lastDesync().has_value());
    REQUIRE_FALSE(second.lastDesync().has_value());
}
