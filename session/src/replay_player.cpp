#include <unison/session/replay_player.hpp>

#include <unison/core/contract.hpp>

namespace unison::session
{

namespace
{

constexpr std::size_t kLocalSlot = 0;

}

ReplayPlayer::ReplayPlayer(sim::Frame& frame, const sim::SystemPipeline& pipeline, const net::SessionConfig& config)
    : session{frame, pipeline, config, kLocalSlot}
{
}

tl::expected<std::optional<VerifiedChecksum>, Error> ReplayPlayer::play(const ReplayFrame& frame)
{
    if (frame.frameNumber != session.verifiedFrame() + 1)
    {
        return tl::unexpected{Error{ErrorCode::MalformedReplay, "the replay's frames do not follow one another"}};
    }

    const bool isConfirmed = session.confirm(frame.frameNumber, frame.inputs);
    session.tick();

    UNISON_VERIFY(isConfirmed && session.verifiedFrame() == frame.frameNumber);

    const std::span<const VerifiedChecksum> taken = session.verifiedChecksums();
    const std::optional<VerifiedChecksum> checksum = taken.empty() ? std::nullopt : std::optional{taken.front()};
    session.clearVerifiedChecksums();
    session.clearEventChanges();

    return checksum;
}

}
