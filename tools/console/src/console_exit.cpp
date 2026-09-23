#include <unison/console/console_exit.hpp>

namespace unison::console
{

namespace
{

constexpr int kPlayedThrough = 0;
constexpr int kLostTheRelay = 1;
constexpr int kOutOfStep = 2;

}

int exitCodeOf(const ConsoleStatus& status)
{
    if (status.desync.has_value())
    {
        return kOutOfStep;
    }

    return status.state == session::ConnectionState::Disconnected ? kLostTheRelay : kPlayedThrough;
}

}
