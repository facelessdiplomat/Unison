#pragma once

#include <unison/core/error.hpp>
#include <unison/session/verified_frame_receiver.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <vector>

namespace unison::session
{

/// How many frames back a desync dumper keeps snapshots for: more than the relay keeps a frame waiting for its
/// checksums, so the frame of any desync it reports is still kept.
inline constexpr std::uint32_t kDumpedFramesKept = 128;

/// Keeps the serialised snapshots of the frames a session checksummed in the last `kDumpedFramesKept` frames, so a
/// client that hears of a desync at one of them can write down the very state it checksummed.
class DesyncDumper final : public IVerifiedFrameReceiver
{
public:
    explicit DesyncDumper(std::filesystem::path directory);

    void frameVerified(const VerifiedFrame& frame) override;

    /// Writes the kept snapshot of a frame into `desync_<frame>_<slot>.snapshot` in the directory, made if missing,
    /// and gives back its path; a frame not kept, or a file that cannot be written, is reported.
    [[nodiscard]] tl::expected<std::filesystem::path, Error> dump(std::uint32_t frameNumber, std::uint8_t slot) const;

private:
    struct KeptSnapshot
    {
        std::uint32_t frameNumber = 0;
        std::vector<std::byte> bytes;
    };

    std::filesystem::path directory;
    std::deque<KeptSnapshot> kept;
};

}
