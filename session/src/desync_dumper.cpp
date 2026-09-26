#include <unison/session/desync_dumper.hpp>

#include <unison/session/file_bytes.hpp>
#include <unison/session/snapshot_serializer.hpp>

#include <algorithm>
#include <format>
#include <system_error>
#include <utility>

namespace unison::session
{

DesyncDumper::DesyncDumper(std::filesystem::path directory) : directory{std::move(directory)}
{
}

void DesyncDumper::frameVerified(const VerifiedFrame& frame)
{
    if (!frame.checksum.has_value())
    {
        return;
    }

    KeptSnapshot reused;

    while (!kept.empty() && kept.front().frameNumber + kDumpedFramesKept < frame.frameNumber)
    {
        reused = std::move(kept.front());
        kept.pop_front();
    }

    reused.frameNumber = frame.frameNumber;
    serializeSnapshot(frame.snapshot, reused.bytes);
    kept.push_back(std::move(reused));
}

tl::expected<std::filesystem::path, Error> DesyncDumper::dump(std::uint32_t frameNumber, std::uint8_t slot) const
{
    const auto snapshot = std::ranges::find_if(
        kept, [frameNumber](const KeptSnapshot& candidate) { return candidate.frameNumber == frameNumber; });

    if (snapshot == kept.end())
    {
        return tl::unexpected{Error{ErrorCode::FrameNotKept, "the snapshot of that frame is no longer kept"}};
    }

    std::error_code ignored;
    std::filesystem::create_directories(directory, ignored);

    const std::filesystem::path path = directory / std::format("desync_{}_{}.snapshot", frameNumber, slot);

    return writeFileBytes(path, snapshot->bytes).map([&path] { return path; });
}

}
