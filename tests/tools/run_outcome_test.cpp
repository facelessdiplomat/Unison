#include <unison/runner/run_outcome.hpp>

#include <unison/runner/runner_options.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <string>

namespace
{

unison::runner::RunOutcome flawless()
{
    unison::runner::RunOutcome outcome;
    outcome.isComplete = true;
    outcome.hostFrames = 603;
    outcome.fewestVerifiedFrames = 600;
    outcome.deepestRollback = 3;
    outcome.predictionWindow = 10;
    outcome.slots = {0, 1};
    outcome.framesCompared = 600;

    return outcome;
}

unison::runner::RunOutcome partingAtFrame42()
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.disagreement = unison::runner::Disagreement{42, {0x1234, 0xabcd}};

    return outcome;
}

}

TEST_CASE("a run that verified every frame alike within its window ends with nought")
{
    REQUIRE(unison::runner::exitCodeOf(flawless()) == 0);
}

TEST_CASE("a run whose clients parted ways ends with two, whatever else went wrong")
{
    unison::runner::RunOutcome outcome = partingAtFrame42();
    outcome.isComplete = false;
    outcome.deepestRollback = 11;

    REQUIRE(unison::runner::exitCodeOf(outcome) == 2);
}

TEST_CASE("a run that rolled back deeper than its window ends with three")
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.deepestRollback = 11;

    REQUIRE(unison::runner::exitCodeOf(outcome) == 3);
}

TEST_CASE("a run that did not verify every frame ends with one")
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.isComplete = false;

    REQUIRE(unison::runner::exitCodeOf(outcome) == 1);
}

TEST_CASE("the report of a flawless run says how many players verified how many frames")
{
    const std::string report = unison::runner::reportOf(flawless(), unison::runner::RunnerOptions{});

    REQUIRE(report.find("2 players verified 600 frames in 603 host frames") != std::string::npos);
}

TEST_CASE("the report of a flawless run says how many frames the clients compared")
{
    const std::string report = unison::runner::reportOf(flawless(), unison::runner::RunnerOptions{});

    REQUIRE(report.find("the clients agree on the checksums of 600 frames") != std::string::npos);
}

TEST_CASE("the report of a desync names the frame and every slot's checksum of it")
{
    const std::string report = unison::runner::reportOf(partingAtFrame42(), unison::runner::RunnerOptions{});

    REQUIRE(report.find("frame 42") != std::string::npos);
    REQUIRE(report.find("slot 0: 0000000000001234") != std::string::npos);
    REQUIRE(report.find("slot 1: 000000000000abcd") != std::string::npos);
}

TEST_CASE("reporting a desync without the slot of every checksum breaks a contract")
{
    unison::runner::RunOutcome outcome = partingAtFrame42();
    outcome.slots = {0};
    const unison::test::FatalHandlerProbe probe;

    static_cast<void>(unison::runner::reportOf(outcome, unison::runner::RunnerOptions{}));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("the report of an overflow names the depth and the window")
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.deepestRollback = 11;

    const std::string report = unison::runner::reportOf(outcome, unison::runner::RunnerOptions{});

    REQUIRE(report.find("11 frames") != std::string::npos);
    REQUIRE(report.find("window of 10") != std::string::npos);
}
