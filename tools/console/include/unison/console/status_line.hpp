#pragma once

#include <unison/net/protocol.hpp>
#include <unison/session/networked_session.hpp>

#include <cstdint>
#include <string>

namespace unison::console
{

/// What a console tells about its client once a second: the name it shows, where it stands with the relay, and
/// in a match its slot, its verified and predicted frames, the rollbacks of the last second and the round trip.
struct ConsoleStatus
{
    std::string name;
    session::ConnectionState state = session::ConnectionState::Idle;
    std::uint8_t slot = net::kNoSlot;
    std::uint32_t verifiedFrame = 0;
    std::uint32_t predictedFrame = 0;
    std::uint32_t rollbacksLastSecond = 0;
    std::uint64_t roundTripMicroseconds = 0;
};

/// The line a console prints about its client, the round trip in whole milliseconds.
[[nodiscard]] std::string statusLineOf(const ConsoleStatus& status);

}
