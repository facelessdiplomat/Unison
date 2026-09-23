#include <unison/runner/checksum_ledger.hpp>

#include <catch2/catch_test_macros.hpp>

#include <support/fatal_handler_probe.hpp>

#include <cstdint>
#include <optional>
#include <vector>

TEST_CASE("clients that report the same checksums agree")
{
    unison::runner::ChecksumLedger ledger{2};
    ledger.record(0, 1, 11);
    ledger.record(1, 1, 11);
    ledger.record(0, 2, 22);
    ledger.record(1, 2, 22);

    REQUIRE_FALSE(ledger.firstDisagreement().has_value());
    REQUIRE(ledger.framesReportedByAll() == 2U);
}

TEST_CASE("the first frame clients report different checksums for is where they part ways")
{
    unison::runner::ChecksumLedger ledger{3};

    for (std::uint32_t frame = 1; frame <= 5; ++frame)
    {
        ledger.record(0, frame, frame * 10U);
        ledger.record(1, frame, frame >= 4 ? 99U : frame * 10U);
        ledger.record(2, frame, frame * 10U);
    }

    const std::optional<unison::runner::Disagreement> disagreement = ledger.firstDisagreement();

    REQUIRE(disagreement.has_value());
    REQUIRE(disagreement->frame == 4U);
    REQUIRE(disagreement->checksums == std::vector<std::uint64_t>{40, 99, 40});
}

TEST_CASE("a frame only some clients have reported is left out of the comparison")
{
    unison::runner::ChecksumLedger ledger{2};
    ledger.record(0, 1, 11);
    ledger.record(1, 1, 11);
    ledger.record(0, 2, 22);

    REQUIRE_FALSE(ledger.firstDisagreement().has_value());
    REQUIRE(ledger.framesReportedByAll() == 1U);
}

TEST_CASE("recording for a client the run does not have breaks a contract")
{
    unison::runner::ChecksumLedger ledger{2};
    const unison::test::FatalHandlerProbe probe;

    ledger.record(2, 1, 11);

    REQUIRE(probe.failureCount() == 1U);
}
