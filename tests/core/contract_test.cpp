#include <unison/core/contract.hpp>
#include <unison/core/log_sink.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace
{

std::size_t fatalCount = 0;
std::string lastFatalMessage;

void recordingFatalHandler(std::string_view message)
{
    ++fatalCount;
    lastFatalMessage = std::string{message};
}

class ScopedFatalHandler
{
public:
    explicit ScopedFatalHandler(unison::FatalHandler handler) : previousHandler{unison::installedFatalHandler()}
    {
        unison::installFatalHandler(handler);
        fatalCount = 0;
        lastFatalMessage.clear();
    }

    ~ScopedFatalHandler()
    {
        unison::installFatalHandler(previousHandler);
    }

    ScopedFatalHandler(const ScopedFatalHandler&) = delete;
    ScopedFatalHandler& operator=(const ScopedFatalHandler&) = delete;
    ScopedFatalHandler(ScopedFatalHandler&&) = delete;
    ScopedFatalHandler& operator=(ScopedFatalHandler&&) = delete;

private:
    unison::FatalHandler previousHandler;
};

#ifdef NDEBUG
constexpr bool kAssertsAreActive = false;
#else
constexpr bool kAssertsAreActive = true;
#endif

bool countEvaluation(std::size_t& evaluations)
{
    ++evaluations;

    return true;
}

}

TEST_CASE("a passing verify leaves the fatal handler alone")
{
    const ScopedFatalHandler installed{recordingFatalHandler};

    UNISON_VERIFY(1 + 1 == 2);

    REQUIRE(fatalCount == 0U);
}

TEST_CASE("a failing verify invokes the installed fatal handler")
{
    const ScopedFatalHandler installed{recordingFatalHandler};

    UNISON_VERIFY(1 + 1 == 3);

    REQUIRE(fatalCount == 1U);
}

TEST_CASE("a failing verify names the condition and where it stood")
{
    const ScopedFatalHandler installed{recordingFatalHandler};

    UNISON_VERIFY(1 + 1 == 3);

    REQUIRE(lastFatalMessage.find("1 + 1 == 3") != std::string::npos);
    REQUIRE(lastFatalMessage.find("contract_test.cpp") != std::string::npos);
}

TEST_CASE("a failing verify also reports through the log sink")
{
    const ScopedFatalHandler installed{recordingFatalHandler};
    const unison::LogSink previousSink = unison::installedLogSink();
    static std::size_t loggedErrors = 0;

    loggedErrors = 0;
    unison::installLogSink(
        [](unison::LogLevel level, std::string_view)
        {
            if (level == unison::LogLevel::Error)
            {
                ++loggedErrors;
            }
        });

    UNISON_VERIFY(1 + 1 == 3);

    unison::installLogSink(previousSink);

    REQUIRE(loggedErrors == 1U);
}

TEST_CASE("a failing assert reports in a debug build and is absent from a release build")
{
    const ScopedFatalHandler installed{recordingFatalHandler};

    UNISON_ASSERT(1 + 1 == 3);

#ifdef NDEBUG
    REQUIRE(fatalCount == 0U);
#else
    REQUIRE(fatalCount == 1U);
#endif
}

TEST_CASE("an assert does not evaluate its condition in a release build")
{
    std::size_t evaluations = 0;

    UNISON_ASSERT(countEvaluation(evaluations));

#ifdef NDEBUG
    REQUIRE(evaluations == 0U);
#else
    REQUIRE(evaluations == 1U);
#endif
}

TEST_CASE("assert is active in exactly the configuration that uses the debug runtime")
{
#ifdef _DEBUG
    STATIC_REQUIRE(kAssertsAreActive);
#else
    STATIC_REQUIRE_FALSE(kAssertsAreActive);
#endif
}
