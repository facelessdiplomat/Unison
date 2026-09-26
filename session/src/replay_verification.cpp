#include <unison/session/replay_verification.hpp>

#include <optional>
#include <variant>

namespace unison::session
{

tl::expected<ReplayVerdict, Error> verifyReplay(ReplayReader& reader, ReplayPlayer& player)
{
    ReplayVerdict verdict;
    std::optional<VerifiedChecksum> lastTaken;

    while (!reader.isAtEnd())
    {
        const tl::expected<ReplayRecord, Error> record = reader.next();

        if (!record.has_value())
        {
            return tl::unexpected{record.error()};
        }

        if (const auto* frame = std::get_if<ReplayFrame>(&*record))
        {
            const tl::expected<std::optional<VerifiedChecksum>, Error> taken = player.play(*frame);

            if (!taken.has_value())
            {
                return tl::unexpected{taken.error()};
            }

            ++verdict.framesPlayed;
            lastTaken = *taken;

            continue;
        }

        const auto* recorded = std::get_if<VerifiedChecksum>(&*record);

        if (recorded == nullptr || !lastTaken.has_value() || lastTaken->frameNumber != recorded->frameNumber)
        {
            return tl::unexpected{
                Error{ErrorCode::MalformedReplay, "the replay holds a checksum of a frame it did not just play"}};
        }

        ++verdict.checksumsCompared;

        if (lastTaken->checksum == recorded->checksum)
        {
            ++verdict.checksumsMatched;
        }
        else if (!verdict.firstDivergentFrame.has_value())
        {
            verdict.firstDivergentFrame = recorded->frameNumber;
        }

        lastTaken.reset();
    }

    return verdict;
}

}
