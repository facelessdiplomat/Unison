#include <unison/core/contract.hpp>
#include <unison/core/log_sink.hpp>

#include <support/fatal_handler_probe.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace
{

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
    const unison::test::FatalHandlerProbe probe;

    UNISON_VERIFY(1 + 1 == 2);

    REQUIRE(probe.failureCount() == 0U);
}

TEST_CASE("a failing verify invokes the installed fatal handler")
{
    const unison::test::FatalHandlerProbe probe;

    UNISON_VERIFY(1 + 1 == 3);

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("a failing verify names the condition and where it stood")
{
    const unison::test::FatalHandlerProbe probe;

    UNISON_VERIFY(1 + 1 == 3);

    REQUIRE(probe.lastMessage().find("1 + 1 == 3") != std::string_view::npos);
    REQUIRE(probe.lastMessage().find("contract_test.cpp") != std::string_view::npos);
}

TEST_CASE("a failing verify also reports through the log sink")
{
    const unison::test::FatalHandlerProbe probe;
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
    const unison::test::FatalHandlerProbe probe;

    UNISON_ASSERT(1 + 1 == 3);

    REQUIRE(probe.failureCount() == (kAssertsAreActive ? 1U : 0U));
}

TEST_CASE("an assert does not evaluate its condition in a release build")
{
    std::size_t evaluations = 0;

    UNISON_ASSERT(countEvaluation(evaluations));

    REQUIRE(evaluations == (kAssertsAreActive ? 1U : 0U));
}

TEST_CASE("assert is active in exactly the configuration that uses the debug runtime")
{
#ifdef _DEBUG
    STATIC_REQUIRE(kAssertsAreActive);
#else
    STATIC_REQUIRE_FALSE(kAssertsAreActive);
#endif
}
