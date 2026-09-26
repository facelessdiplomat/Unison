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
    const std::string firstDivergence =
        verdict.firstDivergentFrame.has_value()
            ? std::format("; the first to differ is frame {}", *verdict.firstDivergentFrame)
            : std::string{};

    return std::format("unison_replay: {} of {} checksums match over {} frames{}\n",
                       verdict.checksumsMatched,
                       verdict.checksumsCompared,
                       verdict.framesPlayed,
                       firstDivergence);
}

int diffExitCodeOf(const std::optional<session::StateDifference>& difference)
{
    return difference.has_value() ? kChecksumDiffered : kEveryChecksumMatched;
}

std::string diffReportOf(const std::optional<session::StateDifference>& difference)
{
    if (!difference.has_value())
    {
        return "unison_replay: the snapshots are alike\n";
    }

    const std::string field = difference->field.has_value() ? std::format(", in its field {}", *difference->field) : "";

    if (difference->entity.has_value() && difference->byte.has_value())
    {
        return std::format("unison_replay: the snapshots first differ in {} of entity {} at byte {}{}\n",
                           difference->part,
                           *difference->entity,
                           *difference->byte,
                           field);
    }

    if (difference->entity.has_value())
    {
        return std::format(
            "unison_replay: the snapshots first differ in {}, where the first holds entity {} and the second another\n",
            difference->part,
            *difference->entity);
    }

    return std::format(
        "unison_replay: the snapshots first differ in {} at byte {}\n", difference->part, difference->byte.value_or(0));
}

std::string playReportOf(const session::ReplayVerdict& verdict, std::uint64_t lastChecksum)
{
    return std::format(
        "unison_replay: played {} frames; the last one's checksum is {:#018x}\n", verdict.framesPlayed, lastChecksum);
}

}
