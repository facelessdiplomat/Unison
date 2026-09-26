#pragma once

#include <unison/core/error.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/replay_format.hpp>
#include <unison/session/session.hpp>
#include <unison/session/verified_checksum.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <tl/expected.hpp>

#include <optional>

namespace unison::session
{

/// Plays a replay back through a session that is handed each frame's inputs before it plays the frame, so it
/// never predicts and verifies every frame at once. The frame and the pipeline belong to the game, which builds
/// them as the recorded match began; a config no session can play breaks a contract.
class ReplayPlayer
{
public:
    ReplayPlayer(sim::Frame& frame, const sim::SystemPipeline& pipeline, const net::SessionConfig& config);

    /// Plays the frame after the last one played on the inputs the replay recorded and gives back the checksum
    /// the session took of it when the config's interval falls on it; any other frame is refused.
    [[nodiscard]] tl::expected<std::optional<VerifiedChecksum>, Error> play(const ReplayFrame& frame);

private:
    Session session;
};

}
