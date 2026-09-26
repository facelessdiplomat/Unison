#pragma once

#include <unison/core/error.hpp>
#include <unison/session/replay_player.hpp>
#include <unison/session/replay_reader.hpp>
#include <unison/session/verified_checksum.hpp>
#include <unison/sim/frame.hpp>
#include <unison/sim/system_pipeline.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace unison::test
{

using PlayedStep = tl::expected<std::optional<session::VerifiedChecksum>, Error>;

/// The checksums a replay recorded, in the order it holds them; none for bytes that are no replay.
[[nodiscard]] inline std::vector<session::VerifiedChecksum> recordedChecksums(std::span<const std::byte> replay)
{
    tl::expected<session::ReplayReader, Error> reader = session::ReplayReader::open(replay);
    std::vector<session::VerifiedChecksum> recorded;

    while (reader.has_value() && !reader->isAtEnd())
    {
        const tl::expected<session::ReplayRecord, Error> record = reader->next();

        if (!record.has_value())
        {
            break;
        }

        if (const auto* checksum = std::get_if<session::VerifiedChecksum>(&*record))
        {
            recorded.push_back(*checksum);
        }
    }

    return recorded;
}

/// The checksums a replay's frames take when played back on a game built as the recorded match began, up to the
/// first record or frame refused.
[[nodiscard]] inline std::vector<session::VerifiedChecksum>
playedChecksums(std::span<const std::byte> replay, sim::Frame& frame, const sim::SystemPipeline& pipeline)
{
    tl::expected<session::ReplayReader, Error> reader = session::ReplayReader::open(replay);

    if (!reader.has_value())
    {
        return {};
    }

    session::ReplayPlayer player{frame, pipeline, reader->config()};
    std::vector<session::VerifiedChecksum> played;

    while (!reader->isAtEnd())
    {
        const tl::expected<session::ReplayRecord, Error> record = reader->next();

        if (!record.has_value())
        {
            break;
        }

        const auto* replayFrame = std::get_if<session::ReplayFrame>(&*record);

        if (replayFrame == nullptr)
        {
            continue;
        }

        const PlayedStep step = player.play(*replayFrame);

        if (!step.has_value())
        {
            break;
        }

        if (step->has_value())
        {
            played.push_back(**step);
        }
    }

    return played;
}

}
