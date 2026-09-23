#pragma once

#include <unison/net/protocol.hpp>
#include <unison/session/networked_session.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace unison::console
{

/// What a console tells about its client: the name it shows, where it stands with the relay, in a match its
/// slot, its verified and predicted frames, the rollbacks of the last second, the round trip and the lead, and
/// the desync the relay reported, if it reported one.
struct ConsoleStatus
{
    std::string name;
    session::ConnectionState state = session::ConnectionState::Idle;
    std::uint8_t slot = net::kNoSlot;
    std::uint32_t verifiedFrame = 0;
    std::uint32_t predictedFrame = 0;
    std::uint32_t rollbacksLastSecond = 0;
    std::uint64_t roundTripMicroseconds = 0;
    std::int64_t leadMicroseconds = 0;
    std::optional<net::Desync> desync;
};

/// The line a console shows about its client, the round trip and the lead in whole milliseconds, ending with the
/// frame and the slots of a desync.
[[nodiscard]] std::string statusLineOf(const ConsoleStatus& status);

}
