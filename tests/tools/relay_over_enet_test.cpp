#include <unison/relay/relay_rooms.hpp>

#include <unison/net/clock.hpp>
#include <unison/net/enet_transport.hpp>
#include <unison/net/message_codec.hpp>
#include <unison/net/outbox.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <variant>

namespace
{

class WelcomeWatch final : public unison::net::IMessageReceiver
{
public:
    void receive(unison::net::PeerId, unison::net::Channel, std::span<const std::byte> message) override
    {
        const auto decoded = unison::net::decode(message);

        if (decoded.has_value() && std::holds_alternative<unison::net::Welcome>(*decoded))
        {
            welcome = std::get<unison::net::Welcome>(*decoded);
        }
    }

    std::optional<unison::net::Welcome> welcome;
};

}

TEST_CASE("a client that connects over ENet is welcomed into a room of the relay")
{
    auto listening = unison::net::EnetTransport::listen(unison::net::EnetAddress{"127.0.0.1", 0}, 4);
    REQUIRE(listening.has_value());
    const std::unique_ptr<unison::net::EnetTransport> server = std::move(*listening);
    const unison::net::SteadyClock clock;
    unison::relay::RelayRooms rooms{*server, clock, unison::net::RelaySettings{}};
    auto connected = unison::net::EnetTransport::connect(unison::net::EnetAddress{"127.0.0.1", server->port()},
                                                         unison::net::EnetAddress{"127.0.0.1", 0});
    REQUIRE(connected.has_value());
    unison::net::SessionConfig config;
    config.slotCount = 2;
    config.inputSize = 4;
    unison::net::Outbox outbox{*connected->transport};
    WelcomeWatch watch;

    outbox.send(connected->server,
                unison::net::Channel::Reliable,
                unison::net::Hello{unison::net::kProtocolVersion, config, unison::net::Role::Player, 0});
    const auto giveUpAt = std::chrono::steady_clock::now() + std::chrono::seconds{2};

    while (!watch.welcome.has_value() && std::chrono::steady_clock::now() < giveUpAt)
    {
        server->poll(rooms);
        rooms.update();
        connected->transport->poll(watch);
    }

    REQUIRE(watch.welcome.has_value());
    REQUIRE(watch.welcome->slot == 0U);
    REQUIRE(unison::net::hashOf(watch.welcome->config) == unison::net::hashOf(config));
}
