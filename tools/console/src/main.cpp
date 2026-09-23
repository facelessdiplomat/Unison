#include "console_keyboard.hpp"

#include <unison/console/arena_controls.hpp>
#include <unison/console/console_options.hpp>
#include <unison/console/status_line.hpp>

#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>

#include <unison/core/log_sink.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/enet_transport.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/networked_session.hpp>
#include <unison/sim/asset_hash.hpp>
#include <unison/sim/pipeline_hash.hpp>
#include <unison/view/event_dispatcher.hpp>
#include <unison/view/session_runner.hpp>

#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <limits>
#include <optional>
#include <span>
#include <thread>

namespace
{

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;
constexpr std::chrono::milliseconds kIdleBetweenFrames{1};

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

unison::net::SessionConfig configOf(const unison::console::ConsoleOptions& options, const arena::ArenaSimulation& match)
{
    unison::net::SessionConfig config;
    config.slotCount = static_cast<std::uint8_t>(options.players);
    config.inputSize = sizeof(arena::ArenaInput);
    config.seed = 1;
    config.assetHash = unison::sim::hashOf(match.assets());
    config.pipelineHash = unison::sim::hashOf(match.pipeline());

    return config;
}

unison::console::ConsoleStatus statusOf(const unison::console::ConsoleOptions& options,
                                        const unison::session::NetworkedSession& networked,
                                        std::uint32_t rollbacksLastSecond)
{
    unison::console::ConsoleStatus status;
    status.name = options.name;
    status.state = networked.state();
    status.slot = networked.localSlot();
    status.rollbacksLastSecond = rollbacksLastSecond;
    status.roundTripMicroseconds = networked.timeSync().roundTripMicroseconds();

    if (const unison::session::Session* played = networked.session())
    {
        status.verifiedFrame = played->verifiedFrame();
        status.predictedFrame = played->predictedFrame();
    }

    return status;
}

std::uint32_t rollbacksSoFar(const unison::session::NetworkedSession& networked)
{
    const unison::session::Session* played = networked.session();

    return played == nullptr ? 0U : played->rollbackStats().rollbacks;
}

}

int main(int argc, char** argv)
{
    unison::installLogSink(printToStandardOutput);
    static_cast<void>(std::signal(SIGINT, askToStop));
    static_cast<void>(std::signal(SIGTERM, askToStop));

    const tl::expected<unison::console::ConsoleOptions, unison::Error> options =
        unison::console::parseConsoleOptions(std::span<const char* const>{argv, static_cast<std::size_t>(argc)});

    if (!options.has_value())
    {
        unison::logMessage(unison::LogLevel::Error, std::format("unison_console: {}", options.error().message()));

        return 1;
    }

    if (options->isHelpAsked)
    {
        std::fputs(unison::console::consoleHelp().c_str(), stdout);

        return 0;
    }

    arena::ArenaSimulation match{options->players};
    const unison::net::SessionConfig config = configOf(*options, match);
    const std::optional<unison::net::EnetAddress> from =
        options->from.empty() ? std::nullopt : std::optional{unison::net::EnetAddress{options->from, 0}};
    auto connected = unison::net::EnetTransport::connect(unison::net::EnetAddress{options->host, options->port}, from);

    if (!connected.has_value())
    {
        unison::logMessage(unison::LogLevel::Error, std::format("unison_console: {}", connected.error().message()));

        return 1;
    }

    const unison::net::SteadyClock clock;
    unison::session::NetworkedSession networked{
        match.frame(), match.pipeline(), config, *connected->transport, connected->server};
    unison::view::EventDispatcher dispatcher;
    unison::view::SessionRunner runner{networked, dispatcher, clock, config.tickRate};
    unison::console::ConsoleKeyboard keyboard;
    unison::console::HeldKeys keys;
    unison::console::ArenaControls controls;
    const std::uint64_t stopAt = options->runForSeconds > 0 ? options->runForSeconds * kMicrosecondsPerSecond
                                                            : std::numeric_limits<std::uint64_t>::max();
    std::uint64_t nextStatusAt = kMicrosecondsPerSecond;
    std::uint64_t lastInputAt = clock.nowMicroseconds();
    std::uint32_t rollbacksAtLastStatus = 0;

    if (!keyboard.isAvailable())
    {
        unison::logMessage(unison::LogLevel::Info,
                           "unison_console: no console to read the keyboard from, so the player stands still");
    }

    networked.join();

    while (isStopAsked == 0 && clock.nowMicroseconds() < stopAt &&
           networked.state() != unison::session::ConnectionState::Disconnected)
    {
        keyboard.readInto(keys);

        const std::uint64_t now = clock.nowMicroseconds();
        const arena::ArenaInput input = controls.inputFor(keys, now - lastInputAt);

        lastInputAt = now;
        runner.setLocalInput(std::as_bytes(std::span{&input, 1}));
        static_cast<void>(runner.update());
        networked.clearConnectionChanges();

        if (clock.nowMicroseconds() >= nextStatusAt)
        {
            const std::uint32_t rollbacks = rollbacksSoFar(networked);

            unison::logMessage(
                unison::LogLevel::Info,
                unison::console::statusLineOf(statusOf(*options, networked, rollbacks - rollbacksAtLastStatus)));
            rollbacksAtLastStatus = rollbacks;
            nextStatusAt += kMicrosecondsPerSecond;
        }

        std::this_thread::sleep_for(kIdleBetweenFrames);
    }

    unison::logMessage(unison::LogLevel::Info, unison::console::statusLineOf(statusOf(*options, networked, 0)));

    return networked.state() == unison::session::ConnectionState::Disconnected ? 1 : 0;
}
