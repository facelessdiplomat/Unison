#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace unison::runner
{

/// Where the clients of a run parted ways: the first frame they reported different checksums for, and every
/// client's checksum of that frame, in the order of the clients.
struct Disagreement
{
    std::uint32_t frame = 0;
    std::vector<std::uint64_t> checksums;
};

/// The checksums every client of a run reported for the frames it verified, compared frame by frame. A frame
/// is compared once every client has reported it.
class ChecksumLedger
{
public:
    explicit ChecksumLedger(std::size_t clients);

    /// Writes down a client's checksum of a frame; a client the run does not have breaks a contract.
    void record(std::size_t client, std::uint32_t frame, std::uint64_t checksum);

    /// The first frame every client reported but not every one alike; nothing while they all agree.
    [[nodiscard]] std::optional<Disagreement> firstDisagreement() const;

    [[nodiscard]] std::size_t framesReportedByAll() const;

    [[nodiscard]] bool isReportedByAll(std::uint32_t frame) const;

private:
    std::vector<std::map<std::uint32_t, std::uint64_t>> reports;
};

}
