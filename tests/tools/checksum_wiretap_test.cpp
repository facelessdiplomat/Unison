#include <unison/runner/checksum_wiretap.hpp>

#include <unison/net/message_codec.hpp>
#include <unison/runner/checksum_ledger.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace
{

constexpr unison::net::PeerId kFirst{3};
constexpr unison::net::PeerId kSecond{7};

class Relay final : public unison::net::IMessageReceiver
{
public:
    void receive(unison::net::PeerId, unison::net::Channel, std::span<const std::byte>) override
    {
        ++received;
    }

    std::uint32_t received = 0;
};

void send(unison::net::IMessageReceiver& receiver, unison::net::PeerId from, const unison::net::Message& message)
{
    std::array<std::byte, 64> buffer{};
    const auto written = unison::net::encode(message, buffer);

    REQUIRE(written.has_value());
    receiver.receive(from, unison::net::Channel::Reliable, std::span{buffer}.first(*written));
}

}

TEST_CASE("the checksums clients send the relay are written down under the client that sent them")
{
    Relay relay;
    unison::runner::ChecksumLedger ledger{2};
    const std::array peers{kFirst, kSecond};
    unison::runner::ChecksumWiretap wiretap{relay, ledger, peers};

    send(wiretap, kFirst, unison::net::Checksum{1, 11});
    send(wiretap, kSecond, unison::net::Checksum{1, 12});

    const auto disagreement = ledger.firstDisagreement();

    REQUIRE(disagreement.has_value());
    REQUIRE(disagreement->checksums == std::vector<std::uint64_t>{11, 12});
}

TEST_CASE("every message reaches the relay behind the wiretap")
{
    Relay relay;
    unison::runner::ChecksumLedger ledger{2};
    const std::array peers{kFirst, kSecond};
    unison::runner::ChecksumWiretap wiretap{relay, ledger, peers};

    send(wiretap, kFirst, unison::net::Checksum{1, 11});
    send(wiretap, kSecond, unison::net::Ping{5});
    send(wiretap, unison::net::PeerId{40}, unison::net::Checksum{1, 13});

    REQUIRE(relay.received == 3U);
    REQUIRE(ledger.framesReportedByAll() == 0U);
}
