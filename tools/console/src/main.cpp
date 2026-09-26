#include "console_keyboard.hpp"
#include "console_screen.hpp"

#include <unison/console/arena_controls.hpp>
#include <unison/console/console_exit.hpp>
#include <unison/console/console_options.hpp>
#include <unison/console/screen.hpp>
#include <unison/console/status_line.hpp>

#include <arena/arena_input.hpp>
#include <arena/arena_simulation.hpp>
#include <arena/text_map.hpp>

#include <unison/core/log_sink.hpp>
#include <unison/net/clock.hpp>
#include <unison/net/enet_transport.hpp>
#include <unison/net/millisecond_timer.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/desync_dumper.hpp>
#include <unison/session/file_bytes.hpp>
#include <unison/session/networked_session.hpp>
#include <unison/session/replay_writer.hpp>
#include <unison/session/verified_frame_fan_out.hpp>
#include <unison/session/verified_frame_receiver.hpp>
#include <unison/sim/asset_hash.hpp>
#include <unison/sim/pipeline_hash.hpp>
#include <unison/view/event_dispatcher.hpp>
#include <unison/view/session_runner.hpp>

#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <limits>
#include <optional>
#include <span>
#include <thread>
#include <vector>

namespace
{

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;
constexpr std::uint64_t kMicrosecondsPerScreen = 100'000;
constexpr std::chrono::milliseconds kIdleBetweenFrames{1};
constexpr std::uint32_t kNoInputDelay = 0;

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

void logStatus(const unison::console::ConsoleStatus& status)
{
    unison::logMessage(unison::LogLevel::Info,
                       std::format("unison_console: {}", unison::console::statusLineOf(status)));
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
    status.leadMicroseconds = networked.timeSync().leadMicroseconds();
    status.desync = networked.lastDesync();

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

bool isInPlay(const unison::session::NetworkedSession& networked)
{
    return networked.state() != unison::session::ConnectionState::Disconnected && !networked.lastDesync().has_value();
}

class Steering
{
public:
    explicit Steering(std::uint64_t now) : lastInputAt{now}
    {
    }

    [[nodiscard]] bool isKeyboardAvailable() const
    {
        return keyboard.isAvailable();
    }

    [[nodiscard]] arena::ArenaInput inputAt(std::uint64_t now)
    {
        keyboard.readInto(keys);

        const arena::ArenaInput input = controls.inputFor(keys, now - lastInputAt);
        lastInputAt = now;

        return input;
    }

private:
    unison::console::ConsoleKeyboard keyboard;
    unison::console::HeldKeys keys;
    unison::console::ArenaControls controls;
    std::uint64_t lastInputAt;
};

class StatusDisplay
{
public:
    StatusDisplay(const unison::console::ConsoleOptions& options,
                  const unison::session::NetworkedSession& networked,
                  const unison::sim::Frame& frame)
        : options{options}, networked{networked}, frame{frame}
    {
    }

    void update(std::uint64_t now)
    {
        if (now >= nextSecondAt)
        {
            const std::uint32_t rollbacks = rollbacksSoFar(networked);
            rollbacksLastSecond = rollbacks - rollbacksAtLastSecond;
            rollbacksAtLastSecond = rollbacks;
            nextSecondAt += kMicrosecondsPerSecond;

            if (!screen.isAvailable())
            {
                logStatus(statusOf(options, networked, rollbacksLastSecond));
            }
        }

        if (screen.isAvailable() && now >= nextScreenAt)
        {
            screen.show(
                unison::console::screenOf(statusOf(options, networked, rollbacksLastSecond), arena::textMapOf(frame)));
            nextScreenAt = now + kMicrosecondsPerScreen;
        }
    }

private:
    const unison::console::ConsoleOptions& options;
    const unison::session::NetworkedSession& networked;
    const unison::sim::Frame& frame;
    unison::console::ConsoleScreen screen;
    std::uint64_t nextSecondAt = kMicrosecondsPerSecond;
    std::uint64_t nextScreenAt = 0;
    std::uint32_t rollbacksAtLastSecond = 0;
    std::uint32_t rollbacksLastSecond = 0;
};

void playUntilStopped(const unison::console::ConsoleOptions& options,
                      const arena::ArenaSimulation& match,
                      unison::session::NetworkedSession& networked,
                      unison::view::SessionRunner& runner,
                      const unison::net::IClock& clock)
{
    const std::uint64_t stopAt = options.runForSeconds > 0 ? options.runForSeconds * kMicrosecondsPerSecond
                                                           : std::numeric_limits<std::uint64_t>::max();
    Steering steering{clock.nowMicroseconds()};

    if (!steering.isKeyboardAvailable())
    {
        unison::logMessage(unison::LogLevel::Info,
                           "unison_console: no console to read the keyboard from, so the player stands still");
    }

    StatusDisplay display{options, networked, match.frame()};
    const unison::net::MillisecondTimer millisecondTimer;
    networked.join();

    while (isStopAsked == 0 && clock.nowMicroseconds() < stopAt && isInPlay(networked))
    {
        const arena::ArenaInput input = steering.inputAt(clock.nowMicroseconds());

        runner.setLocalInput(std::as_bytes(std::span{&input, 1}));
        static_cast<void>(runner.update());
        networked.clearConnectionChanges();
        display.update(clock.nowMicroseconds());
        std::this_thread::sleep_for(kIdleBetweenFrames);
    }
}

class MatchKeeping
{
public:
    MatchKeeping(const unison::console::ConsoleOptions& options, const unison::net::SessionConfig& config)
        : options{options}, recording{recordingFor(options, config)}, dumper{dumperFor(options)},
          verifiedFrames{receiversOf(recording, dumper)}
    {
    }

    MatchKeeping(const MatchKeeping&) = delete;
    MatchKeeping& operator=(const MatchKeeping&) = delete;
    MatchKeeping(MatchKeeping&&) = delete;
    MatchKeeping& operator=(MatchKeeping&&) = delete;

    [[nodiscard]] unison::session::IVerifiedFrameReceiver* receiver()
    {
        return &verifiedFrames;
    }

    [[nodiscard]] bool keep(const unison::session::NetworkedSession& networked) const
    {
        const bool isReplayKept = keepReplay();
        const bool isDumpKept = keepDesyncDump(networked);

        return isReplayKept && isDumpKept;
    }

private:
    static std::optional<unison::session::ReplayWriter> recordingFor(const unison::console::ConsoleOptions& options,
                                                                     const unison::net::SessionConfig& config)
    {
        return options.recordPath.empty() ? std::nullopt : std::optional{unison::session::ReplayWriter{config}};
    }

    static std::optional<unison::session::DesyncDumper> dumperFor(const unison::console::ConsoleOptions& options)
    {
        return options.dumpDirectory.empty() ? std::nullopt
                                             : std::optional{unison::session::DesyncDumper{options.dumpDirectory}};
    }

    static std::vector<unison::session::IVerifiedFrameReceiver*>
    receiversOf(std::optional<unison::session::ReplayWriter>& recording,
                std::optional<unison::session::DesyncDumper>& dumper)
    {
        std::vector<unison::session::IVerifiedFrameReceiver*> receivers;

        if (recording.has_value())
        {
            receivers.push_back(&*recording);
        }

        if (dumper.has_value())
        {
            receivers.push_back(&*dumper);
        }

        return receivers;
    }

    [[nodiscard]] bool keepReplay() const
    {
        if (!recording.has_value())
        {
            return true;
        }

        const tl::expected<void, unison::Error> written =
            unison::session::writeFileBytes(options.recordPath, recording->bytes());

        if (!written.has_value())
        {
            unison::logMessage(unison::LogLevel::Error,
                               std::format("unison_console: {}: {}", written.error().message(), options.recordPath));

            return false;
        }

        unison::logMessage(unison::LogLevel::Info,
                           std::format("unison_console: recorded the match into {}", options.recordPath));

        return true;
    }

    [[nodiscard]] bool keepDesyncDump(const unison::session::NetworkedSession& networked) const
    {
        const std::optional<unison::net::Desync> desync = networked.lastDesync();

        if (!dumper.has_value() || !desync.has_value())
        {
            return true;
        }

        const tl::expected<std::filesystem::path, unison::Error> dumped =
            dumper->dump(desync->frame, networked.localSlot());

        if (!dumped.has_value())
        {
            unison::logMessage(unison::LogLevel::Error, std::format("unison_console: {}", dumped.error().message()));

            return false;
        }

        unison::logMessage(unison::LogLevel::Info,
                           std::format("unison_console: dumped the snapshot of the desync into {}", dumped->string()));

        return true;
    }

    const unison::console::ConsoleOptions& options;
    std::optional<unison::session::ReplayWriter> recording;
    std::optional<unison::session::DesyncDumper> dumper;
    unison::session::VerifiedFrameFanOut verifiedFrames;
};

tl::expected<unison::net::EnetConnection, unison::Error> connectToRelay(const unison::console::ConsoleOptions& options)
{
    const std::optional<unison::net::EnetAddress> from =
        options.from.empty() ? std::nullopt : std::optional{unison::net::EnetAddress{options.from, 0}};

    return unison::net::EnetTransport::connect(unison::net::EnetAddress{options.host, options.port}, from);
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
    auto connected = connectToRelay(*options);

    if (!connected.has_value())
    {
        unison::logMessage(unison::LogLevel::Error, std::format("unison_console: {}", connected.error().message()));

        return 1;
    }

    const unison::net::SteadyClock clock;
    MatchKeeping keeping{*options, config};
    unison::session::NetworkedSession networked{match.frame(),
                                                match.pipeline(),
                                                config,
                                                *connected->transport,
                                                connected->server,
                                                kNoInputDelay,
                                                keeping.receiver()};
    unison::view::EventDispatcher dispatcher;
    unison::view::SessionRunner runner{networked, dispatcher, clock, config.tickRate};

    playUntilStopped(*options, match, networked, runner, clock);

    const unison::console::ConsoleStatus lastStatus = statusOf(*options, networked, 0);
    logStatus(lastStatus);
    const int exitCode = unison::console::exitCodeOf(lastStatus);

    if (!keeping.keep(networked) && exitCode == 0)
    {
        return 1;
    }

    return exitCode;
}
