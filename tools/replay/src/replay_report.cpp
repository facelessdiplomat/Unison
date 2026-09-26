#include <unison/replay/replay_report.hpp>

#include <format>

namespace unison::replay
{

namespace
{

constexpr int kEveryChecksumMatched = 0;
constexpr int kChecksumDiffered = 2;

}

tl::expected<void, Error>
checkRecordedContent(const net::SessionConfig& recorded, std::uint64_t assetHash, std::uint64_t pipelineHash)
{
    if (recorded.assetHash != assetHash || recorded.pipelineHash != pipelineHash)
    {
        return tl::unexpected{
            Error{ErrorCode::ForeignReplay, "the replay was recorded with other assets or systems than this build's"}};
    }

    return {};
}

int verifyExitCodeOf(const session::ReplayVerdict& verdict)
{
    return verdict.checksumsMatched == verdict.checksumsCompared ? kEveryChecksumMatched : kChecksumDiffered;
}

std::string verifyReportOf(const session::ReplayVerdict& verdict)
{
    return std::format("unison_replay: {} of {} checksums match over {} frames\n",
                       verdict.checksumsMatched,
                       verdict.checksumsCompared,
                       verdict.framesPlayed);
}

std::string playReportOf(const session::ReplayVerdict& verdict, std::uint64_t lastChecksum)
{
    return std::format(
        "unison_replay: played {} frames; the last one's checksum is {:#018x}\n", verdict.framesPlayed, lastChecksum);
}

}
