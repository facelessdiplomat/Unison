#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/arena_script.hpp>
#include <support/checksum_tap.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/enet_transport.hpp>
#include <unison/relay/relay_rooms.hpp>
#include <unison/session/networked_session.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>

namespace
{

constexpr std::uint8_t kPlayers = 2;
constexpr std::uint32_t kFrames = 1000;
constexpr std::uint32_t kMostHostFrames = 3 * kFrames;
constexpr std::uint32_t kLateJoinAllowance = 30;
constexpr std::uint64_t kHostFrameMicroseconds = 16'667;
constexpr unison::net::PeerId kFirstToConnect{1};
constexpr unison::net::PeerId kSecondToConnect{2};

unison::net::SessionConfig twoPlayerMatch()
{
    unison::net::SessionConfig config;
    config.slotCount = kPlayers;
    config.inputSize = sizeof(arena::ArenaInput);
    config.checksumInterval = 1;

    return config;
}

struct Client
{
    explicit Client(unison::net::EnetConnection connected)
        : connection{std::move(connected)},
          networked{match.frame(), match.pipeline(), twoPlayerMatch(), *connection.transport, connection.server}
    {
    }

    void playHostFrame(std::uint64_t now)
    {
        networked.update(now);

        const std::int32_t ticks = 1 + networked.takeTickCorrection();

        for (std::int32_t tick = 0; tick < ticks; ++tick)
        {
            const std::uint32_t frame = networked.session() == nullptr ? 0 : networked.session()->predictedFrame();
            const std::size_t slot = networked.localSlot() == unison::net::kNoSlot ? 0 : networked.localSlot();

            networked.setLocalInput(unison::test::scriptedInputs(frame).bytesAt(slot).first(sizeof(arena::ArenaInput)));
            networked.tick();
        }
    }

    [[nodiscard]] std::uint32_t verifiedFrame() const
    {
        return networked.session() == nullptr ? 0 : networked.session()->verifiedFrame();
    }

    unison::net::EnetConnection connection;
    arena::ArenaSimulation match{kPlayers};
    unison::session::NetworkedSession networked;
};

std::uint32_t fewestVerifiedFrames(const std::deque<Client>& clients)
{
    std::uint32_t fewest = kFrames;

    for (const Client& client : clients)
    {
        fewest = std::min(fewest, client.verifiedFrame());
    }

    return fewest;
}

unison::test::ChecksumReports
reportsBetween(const unison::test::ChecksumReports& reports, std::uint32_t firstFrame, std::uint32_t lastFrame)
{
    unison::test::ChecksumReports between;

    for (const auto& report : reports)
    {
        if (report.first >= firstFrame && report.first <= lastFrame)
        {
            between.push_back(report);
        }
    }

    std::ranges::sort(between);

    return between;
}

unison::net::EnetConnection connectionTo(const unison::net::EnetTransport& server)
{
    auto connected = unison::net::EnetTransport::connect(unison::net::EnetAddress{"127.0.0.1", server.port()},
                                                         unison::net::EnetAddress{"127.0.0.1", 0});

    REQUIRE(connected.has_value());

    return std::move(*connected);
}

}

TEST_CASE("two clients play a thousand frames through a relay over ENet on localhost and agree on every checksum")
{
    auto listening = unison::net::EnetTransport::listen(unison::net::EnetAddress{"127.0.0.1", 0}, 4);
    REQUIRE(listening.has_value());
    const std::unique_ptr<unison::net::EnetTransport> server = std::move(*listening);
    unison::net::ManualClock clock;
    unison::relay::RelayRooms rooms{*server, clock, unison::net::RelaySettings{}};
    unison::test::ChecksumTap tap{rooms, kFirstToConnect, kSecondToConnect};
    std::deque<Client> clients;
    clients.emplace_back(connectionTo(*server));
    clients.front().networked.join();
    const auto giveUpWelcomingAt = std::chrono::steady_clock::now() + std::chrono::seconds{2};

    while (clients.front().networked.state() != unison::session::ConnectionState::Playing &&
           std::chrono::steady_clock::now() < giveUpWelcomingAt)
    {
        server->poll(tap);
        clients.front().networked.update(clock.nowMicroseconds());
    }

    REQUIRE(clients.front().networked.state() == unison::session::ConnectionState::Playing);
    clients.emplace_back(connectionTo(*server));
    clients.back().networked.join();
    std::uint32_t hostFrames = 0;

    while (hostFrames < kMostHostFrames && fewestVerifiedFrames(clients) < kFrames)
    {
        clock.advance(kHostFrameMicroseconds);
        server->poll(tap);
        rooms.update();

        for (Client& client : clients)
        {
            client.playHostFrame(clock.nowMicroseconds());
        }

        ++hostFrames;
    }

    server->poll(tap);
    REQUIRE_FALSE(tap.firstReports.empty());
    REQUIRE_FALSE(tap.secondReports.empty());
    const std::uint32_t firstCommon = std::max(tap.firstReports.front().first, tap.secondReports.front().first);
    const std::uint32_t lastCommon = std::min(tap.firstReports.back().first, tap.secondReports.back().first);

    REQUIRE(hostFrames < kMostHostFrames);
    REQUIRE(lastCommon >= firstCommon + kFrames - kLateJoinAllowance);
    REQUIRE(reportsBetween(tap.firstReports, firstCommon, lastCommon) ==
            reportsBetween(tap.secondReports, firstCommon, lastCommon));
    REQUIRE_FALSE(clients.front().networked.lastDesync().has_value());
    REQUIRE_FALSE(clients.back().networked.lastDesync().has_value());
}
