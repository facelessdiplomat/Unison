#include <unison/core/log_sink.hpp>

#include <atomic>

namespace unison
{

namespace
{

std::atomic<LogSink> installedSink{nullptr};

}

void installLogSink(LogSink sink)
{
    installedSink.store(sink, std::memory_order_relaxed);
}

LogSink installedLogSink()
{
    return installedSink.load(std::memory_order_relaxed);
}

void logMessage(LogLevel level, std::string_view message)
{
    const LogSink sink = installedSink.load(std::memory_order_relaxed);

    if (sink != nullptr)
    {
        sink(level, message);
    }
}

}
