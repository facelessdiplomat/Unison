#include <unison/core/log_sink.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace
{

struct Record
{
    bool received = false;
    unison::LogLevel level = unison::LogLevel::Debug;
    std::string message;
};

Record lastRecord;
std::size_t countingSinkCalls = 0;

void recordingSink(unison::LogLevel level, std::string_view message)
{
    lastRecord = Record{true, level, std::string{message}};
}

void countingSink(unison::LogLevel, std::string_view)
{
    ++countingSinkCalls;
}

class ScopedLogSink
{
public:
    explicit ScopedLogSink(unison::LogSink sink) : previousSink{unison::installedLogSink()}
    {
        unison::installLogSink(sink);
        lastRecord = Record{};
        countingSinkCalls = 0;
    }

    ~ScopedLogSink()
    {
        unison::installLogSink(previousSink);
    }

    ScopedLogSink(const ScopedLogSink&) = delete;
    ScopedLogSink& operator=(const ScopedLogSink&) = delete;
    ScopedLogSink(ScopedLogSink&&) = delete;
    ScopedLogSink& operator=(ScopedLogSink&&) = delete;

private:
    unison::LogSink previousSink;
};

}

TEST_CASE("log sink receives the level and the message")
{
    const ScopedLogSink installed{recordingSink};

    unison::logMessage(unison::LogLevel::Warning, "desync at frame 42");

    REQUIRE(lastRecord.received);
    REQUIRE(lastRecord.level == unison::LogLevel::Warning);
    REQUIRE(lastRecord.message == "desync at frame 42");
}

TEST_CASE("log sink is silent when none is installed")
{
    const ScopedLogSink installed{nullptr};

    unison::logMessage(unison::LogLevel::Error, "nobody is listening");

    REQUIRE_FALSE(lastRecord.received);
    REQUIRE(countingSinkCalls == 0U);
}

TEST_CASE("log sink replaces the one installed before it")
{
    const ScopedLogSink installed{recordingSink};

    unison::installLogSink(countingSink);
    unison::logMessage(unison::LogLevel::Info, "only the second sink hears this");

    REQUIRE_FALSE(lastRecord.received);
    REQUIRE(countingSinkCalls == 1U);
}

TEST_CASE("log sink can be read back after installing it")
{
    const ScopedLogSink installed{recordingSink};

    REQUIRE(unison::installedLogSink() == recordingSink);
}
