#include <unison/relay/relay_options.hpp>
#include <unison/relay/relay_rooms.hpp>

#include <unison/core/log_sink.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/enet_transport.hpp>
#include <unison/net/millisecond_timer.hpp>
#include <unison/net/relay_core.hpp>

#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <limits>
#include <memory>
#include <span>
#include <thread>

namespace
{

constexpr std::uint64_t kMicrosecondsPerMillisecond = 1'000;
constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;
constexpr std::chrono::milliseconds kIdleBetweenPolls{1};

volatile std::sig_atomic_t isStopAsked = 0;

void askToStop(int)
{
    isStopAsked = 1;
}

void printToStandardOutput(unison::LogLevel, std::string_view message)
{
    std::fprintf(stdout, "%.*s\n", static_cast<int>(message.size()), message.data());
    std::fflush(stdout);
}

void relayUntil(std::uint64_t stopAt,
                unison::net::ITransport& transport,
                unison::relay::RelayRooms& rooms,
                const unison::net::IClock& clock)
{
    const unison::net::MillisecondTimer millisecondTimer;

    while (isStopAsked == 0 && clock.nowMicroseconds() < stopAt)
    {
        transport.poll(rooms);
        rooms.update();
        std::this_thread::sleep_for(kIdleBetweenPolls);
    }
}

}

int main(int argc, char** argv)
{
    unison::installLogSink(printToStandardOutput);
    static_cast<void>(std::signal(SIGINT, askToStop));
    static_cast<void>(std::signal(SIGTERM, askToStop));

    const tl::expected<unison::relay::RelayOptions, unison::Error> options =
        unison::relay::parseRelayOptions(std::span<const char* const>{argv, static_cast<std::size_t>(argc)});

    if (!options.has_value())
    {
        unison::logMessage(unison::LogLevel::Error, std::format("unison_relay: {}", options.error().message()));

        return 1;
    }

    if (options->isHelpAsked)
    {
        std::fputs(unison::relay::relayHelp().c_str(), stdout);

        return 0;
    }

    auto listening = unison::net::EnetTransport::listen(unison::net::EnetAddress{options->bindAddress, options->port},
                                                        options->maxPeers,
                                                        std::chrono::milliseconds{options->peerTimeoutMilliseconds});

    if (!listening.has_value())
    {
        unison::logMessage(unison::LogLevel::Error, std::format("unison_relay: {}", listening.error().message()));

        return 1;
    }

    const std::unique_ptr<unison::net::EnetTransport> transport = std::move(*listening);
    const unison::net::SteadyClock clock;
    unison::relay::RelayRooms rooms{
        *transport,
        clock,
        unison::net::RelaySettings{options->inputDeadlineMilliseconds * kMicrosecondsPerMillisecond,
                                   options->reliableResendInterval},
        transport.get()};

    unison::logMessage(unison::LogLevel::Info,
                       std::format("unison_relay: listening on {}:{}", options->bindAddress, transport->port()));

    const std::uint64_t stopAt = options->runForSeconds > 0
                                     ? clock.nowMicroseconds() + options->runForSeconds * kMicrosecondsPerSecond
                                     : std::numeric_limits<std::uint64_t>::max();

    relayUntil(stopAt, *transport, rooms, clock);

    unison::logMessage(unison::LogLevel::Info, "unison_relay: stopped");

    return 0;
}
