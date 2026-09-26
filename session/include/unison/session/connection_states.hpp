#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace unison::session
{

/// Where a client stands with the relay: not asked to join yet, waiting for its transport to reach the
/// relay, waiting to be let in, playing in the slot it was given, playing but waiting for the relay with its
/// prediction window full, or sent away, left behind by a relay that has gone, or fallen further behind the relay than
/// it can hold the confirmations of.
enum class ConnectionState : std::uint8_t
{
    Idle,
    Connecting,
    Joining,
    Playing,
    Stalled,
    Disconnected
};

/// Where a client stands with the relay, and every state it has moved into since the host last took them.
class ConnectionStates
{
public:
    [[nodiscard]] ConnectionState current() const;

    /// Moves into a state, noting the change unless the client stands there already.
    void moveTo(ConnectionState next);

    /// Every state moved into since the changes were last cleared, oldest first.
    [[nodiscard]] std::span<const ConnectionState> changes() const;

    void clearChanges();

    /// Whether the client plays a match: playing, or stalled with its prediction window full.
    [[nodiscard]] bool isInMatch() const;

private:
    ConnectionState state = ConnectionState::Idle;
    std::vector<ConnectionState> moves;
};

}
