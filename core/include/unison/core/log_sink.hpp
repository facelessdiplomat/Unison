#pragma once

#include <cstdint>
#include <string_view>

namespace unison
{

/// Severity of a log message, ordered from the most talkative to the most serious.
enum class LogLevel : std::uint8_t
{
    Debug,
    Info,
    Warning,
    Error
};

/// Callback a host installs to receive Unison's log messages. The message is not null-terminated and
/// does not outlive the call, so a sink that keeps it must copy it.
using LogSink = void (*)(LogLevel level, std::string_view message);

/// Installs the process-wide sink, replacing any previous one; a null sink makes logging silent.
void installLogSink(LogSink sink);

/// Returns the process-wide sink, or null when none is installed.
[[nodiscard]] LogSink installedLogSink();

/// Passes one message to the installed sink, and does nothing when none is installed.
void logMessage(LogLevel level, std::string_view message);

}
