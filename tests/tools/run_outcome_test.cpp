#include <unison/runner/run_outcome.hpp>

#include <unison/net/protocol.hpp>
#include <unison/runner/client_outcome.hpp>
#include <unison/runner/runner_options.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstdint>
#include <string>

namespace
{

unison::runner::ClientOutcome clientInSlot(std::uint8_t slot, std::uint32_t deepestRollback)
{
    unison::runner::ClientOutcome client;
    client.slot = slot;
    client.rollbacks.rollbacks = 40;
    client.rollbacks.deepestRollback = deepestRollback;
    client.rollbacks.resimulatedFrames = 80;
    client.rollbacks.framesPlayed = 600;

    return client;
}

unison::runner::RunOutcome flawless()
{
    unison::runner::RunOutcome outcome;
    outcome.isComplete = true;
    outcome.hostFrames = 603;
    outcome.fewestVerifiedFrames = 600;
    outcome.predictionWindow = 10;
    outcome.clients = {clientInSlot(0, 3), clientInSlot(1, 2)};
    outcome.framesCompared = 600;

    return outcome;
}

unison::runner::RunOutcome partingAtFrame42()
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.disagreement = unison::runner::Disagreement{42, {0x1234, 0xabcd}};

    return outcome;
}

unison::runner::RunOutcome overflowing()
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.clients[1].rollbacks.deepestRollback = 11;

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
    outcome.clients[1].rollbacks.deepestRollback = 11;

    REQUIRE(unison::runner::exitCodeOf(outcome) == 2);
}

TEST_CASE("a run in which any client rolled back deeper than its window ends with three")
{
    REQUIRE(unison::runner::exitCodeOf(overflowing()) == 3);
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
    outcome.clients.pop_back();
    const unison::test::FatalHandlerProbe probe;

    static_cast<void>(unison::runner::reportOf(outcome, unison::runner::RunnerOptions{}));

    REQUIRE(probe.failureCount() == 1U);
}

TEST_CASE("the report of an overflow names the deepest rollback and the window")
{
    const std::string report = unison::runner::reportOf(overflowing(), unison::runner::RunnerOptions{});

    REQUIRE(report.find("11 frames") != std::string::npos);
    REQUIRE(report.find("window of 10") != std::string::npos);
}

TEST_CASE("the report of every run closes with its rollback summary")
{
    const std::string flawlessReport = unison::runner::reportOf(flawless(), unison::runner::RunnerOptions{});
    const std::string desyncReport = unison::runner::reportOf(partingAtFrame42(), unison::runner::RunnerOptions{});

    REQUIRE(flawlessReport.find("mean depth") != std::string::npos);
    REQUIRE(desyncReport.find("mean depth") != std::string::npos);
}

TEST_CASE("the report of a run names every player that joined late and the frame of the snapshot it started from")
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.clients[1].startFrame = 312;

    const std::string report = unison::runner::reportOf(outcome, unison::runner::RunnerOptions{});

    REQUIRE(report.find("slot 1 joined late, from a snapshot of frame 312") != std::string::npos);
    REQUIRE(report.find("slot 0 joined late") == std::string::npos);
}

TEST_CASE("the report of a run names every player that came back and the frame of the snapshot it started from")
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.clients[1].startFrame = 312;
    outcome.clients[1].hasComeBack = true;

    const std::string report = unison::runner::reportOf(outcome, unison::runner::RunnerOptions{});

    REQUIRE(report.find("slot 1 came back, from a snapshot of frame 312") != std::string::npos);
    REQUIRE(report.find("joined late") == std::string::npos);
}

TEST_CASE("the report of a run with spectators says how many of them verified every frame with the players")
{
    unison::runner::RunOutcome outcome = flawless();
    outcome.clients.push_back(clientInSlot(unison::net::kNoSlot, 0));
    unison::runner::RunnerOptions options;
    options.spectators = 1;

    const std::string report = unison::runner::reportOf(outcome, options);

    REQUIRE(report.find("2 players and 1 spectator verified 600 frames in 603 host frames") != std::string::npos);
}
