#include <unison/session/connection_states.hpp>

namespace unison::session
{

ConnectionState ConnectionStates::current() const
{
    return state;
}

void ConnectionStates::moveTo(ConnectionState next)
{
    if (next == state)
    {
        return;
    }

    state = next;
    moves.push_back(next);
}

std::span<const ConnectionState> ConnectionStates::changes() const
{
    return moves;
}

void ConnectionStates::clearChanges()
{
    moves.clear();
}

bool ConnectionStates::isInMatch() const
{
    return state == ConnectionState::Playing || state == ConnectionState::Stalled;
}

}
