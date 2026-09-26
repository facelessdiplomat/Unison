#include <unison/relay/relay_rooms.hpp>

#include <unison/net/clock.hpp>
#include <unison/net/loopback_hub.hpp>
#include <unison/net/message_codec.hpp>
#include <unison/net/outbox.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fixed_round_trips.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace
{

unison::net::SessionConfig matchOf(std::uint8_t players)
{
    unison::net::SessionConfig config;
    config.slotCount = players;
    config.inputSize = 2;

    return config;
}

class Mailbox final : public unison::net::IMessageReceiver
{
public:
    void receive(unison::net::PeerId, unison::net::Channel, std::span<const std::byte> message) override
    {
        letters.emplace_back(message.begin(), message.end());
    }

    template <typename T>
    [[nodiscard]] std::optional<T> first() const
    {
        for (const std::vector<std::byte>& letter : letters)
        {
            const auto decoded = unison::net::decode(letter);

            if (decoded.has_value() && std::holds_alternative<T>(*decoded))
            {
                return std::get<T>(*decoded);
            }
        }

        return std::nullopt;
    }

    std::vector<std::vector<std::byte>> letters;
};

struct Relay
{
    unison::net::LoopbackHub hub;
    unison::net::LoopbackEndpoint& endpoint = hub.join();
    unison::net::ManualClock clock;
    unison::test::FixedRoundTrips roundTrips;
    unison::relay::RelayRooms rooms{endpoint, clock, unison::net::RelaySettings{}, &roundTrips};

    struct Client
    {
        unison::net::LoopbackEndpoint& endpoint;
        unison::net::Outbox outbox{endpoint};
        Mailbox mail;
    };

    Client& join(const unison::net::SessionConfig& config)
    {
        Client& client = clients.emplace_back(hub.join());
        client.outbox.send(endpoint.id(),
                           unison::net::Channel::Reliable,
                           unison::net::Hello{unison::net::kProtocolVersion, config, unison::net::Role::Player, 0});
        endpoint.poll(rooms);
        client.endpoint.poll(client.mail);

        return client;
    }

    void letTheGraceRunOut()
    {
        clock.advance(unison::net::RelaySettings{}.reconnectGraceMicroseconds);
        rooms.update();
    }

    void sendInput(Client& client, std::uint32_t frame)
    {
        const std::array<std::byte, 2> input{};
        client.outbox.send(endpoint.id(), unison::net::Channel::Unreliable, unison::net::Input{frame, 2, 1, input});
        endpoint.poll(rooms);
    }

    std::deque<Client> clients;
};

}

TEST_CASE("a relay has no room open before the first hello")
{
    const Relay relay;

    REQUIRE(relay.rooms.roomCount() == 0U);
}

TEST_CASE("the first hello of a match opens a room and is welcomed into its first slot")
{
    Relay relay;

    const Relay::Client& client = relay.join(matchOf(2));

    REQUIRE(relay.rooms.roomCount() == 1U);
    REQUIRE(client.mail.first<unison::net::Welcome>().has_value());
    REQUIRE(client.mail.first<unison::net::Welcome>()->slot == 0U);
}

TEST_CASE("a hello of the same match joins the room it has")
{
    Relay relay;
    static_cast<void>(relay.join(matchOf(2)));

    const Relay::Client& second = relay.join(matchOf(2));

    REQUIRE(relay.rooms.roomCount() == 1U);
    REQUIRE(second.mail.first<unison::net::Welcome>()->slot == 1U);
}

TEST_CASE("a hello of another match opens a room of its own")
{
    Relay relay;
    static_cast<void>(relay.join(matchOf(2)));

    const Relay::Client& other = relay.join(matchOf(3));

    REQUIRE(relay.rooms.roomCount() == 2U);
    REQUIRE(other.mail.first<unison::net::Welcome>()->slot == 0U);
}

TEST_CASE("a client in a room is answered by that room")
{
    Relay relay;
    Relay::Client& client = relay.join(matchOf(2));

    client.outbox.send(relay.endpoint.id(), unison::net::Channel::Unreliable, unison::net::Ping{42});
    relay.endpoint.poll(relay.rooms);
    client.endpoint.poll(client.mail);

    REQUIRE(client.mail.first<unison::net::Pong>().has_value());
    REQUIRE(client.mail.first<unison::net::Pong>()->pingSentAt == 42U);
}

TEST_CASE("a peer that has said no hello is not answered and opens no room")
{
    Relay relay;
    unison::net::LoopbackEndpoint& stranger = relay.hub.join();
    unison::net::Outbox outbox{stranger};
    Mailbox mail;

    outbox.send(relay.endpoint.id(), unison::net::Channel::Unreliable, unison::net::Ping{42});
    relay.endpoint.poll(relay.rooms);
    stranger.poll(mail);

    REQUIRE(relay.rooms.roomCount() == 0U);
    REQUIRE(mail.letters.empty());
}

TEST_CASE("a room closes once its last peer has left and the grace of the slots it held has passed")
{
    Relay relay;
    const Relay::Client& first = relay.join(matchOf(2));
    const Relay::Client& second = relay.join(matchOf(2));
    relay.rooms.peerLeft(first.endpoint.id());
    relay.rooms.peerLeft(second.endpoint.id());
    const std::size_t withinTheGrace = relay.rooms.roomCount();

    relay.letTheGraceRunOut();

    REQUIRE(withinTheGrace == 1U);
    REQUIRE(relay.rooms.roomCount() == 0U);
}

TEST_CASE("a room whose only client, a spectator, has left closes at once")
{
    Relay relay;
    Relay::Client& spectator = relay.clients.emplace_back(relay.hub.join());
    spectator.outbox.send(
        relay.endpoint.id(),
        unison::net::Channel::Reliable,
        unison::net::Hello{unison::net::kProtocolVersion, matchOf(2), unison::net::Role::Spectator, 0});
    relay.endpoint.poll(relay.rooms);

    relay.rooms.peerLeft(spectator.endpoint.id());

    REQUIRE(relay.rooms.roomCount() == 0U);
}

TEST_CASE("the room count follows joins and leaves across matches")
{
    Relay relay;
    const Relay::Client& ofTwo = relay.join(matchOf(2));
    static_cast<void>(relay.join(matchOf(3)));
    const std::size_t withBoth = relay.rooms.roomCount();
    relay.rooms.peerLeft(ofTwo.endpoint.id());
    relay.letTheGraceRunOut();
    const std::size_t afterTheFirstClosed = relay.rooms.roomCount();

    const Relay::Client& ofTwoAgain = relay.join(matchOf(2));

    REQUIRE(withBoth == 2U);
    REQUIRE(afterTheFirstClosed == 1U);
    REQUIRE(relay.rooms.roomCount() == 2U);
    REQUIRE(ofTwoAgain.mail.first<unison::net::Welcome>()->slot == 0U);
}

TEST_CASE("a peer that leaves without having said hello changes nothing")
{
    Relay relay;
    static_cast<void>(relay.join(matchOf(2)));

    relay.rooms.peerLeft(unison::net::PeerId{99});

    REQUIRE(relay.rooms.roomCount() == 1U);
}

TEST_CASE("a room asks the player its meter measures lowest for the snapshot of a player joining late")
{
    Relay relay;
    Relay::Client& first = relay.join(matchOf(3));
    Relay::Client& second = relay.join(matchOf(3));
    relay.sendInput(first, 1);
    relay.sendInput(second, 1);
    relay.roundTrips.set(first.endpoint.id(), 40'000);
    relay.roundTrips.set(second.endpoint.id(), 10'000);

    static_cast<void>(relay.join(matchOf(3)));
    first.endpoint.poll(first.mail);
    second.endpoint.poll(second.mail);

    REQUIRE_FALSE(first.mail.first<unison::net::SnapshotRequest>().has_value());
    REQUIRE(second.mail.first<unison::net::SnapshotRequest>().has_value());
}
