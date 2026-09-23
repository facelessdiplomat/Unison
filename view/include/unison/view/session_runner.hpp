#pragma once

#include <unison/net/clock.hpp>
#include <unison/session/networked_session.hpp>
#include <unison/view/event_dispatcher.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace unison::view
{

/// What one host frame of a runner did: the ticks it ran, the rollbacks those ticks took, and how far the host's
/// time has gone into the next tick, from nought up to one, for drawing between the last two frames.
struct RunnerStep
{
    std::uint32_t ticks = 0;
    std::uint32_t rollbacks = 0;
    float alpha = 0.0F;
};

/// Drives a networked session from the host's time. Every host frame it takes in what the relay sent, runs as
/// many ticks as the time the host let pass holds, one more or one fewer when the session asks to keep pace
/// with the relay, and hands the events those ticks raised and took back to the dispatcher, which then forgets
/// the frames nothing can take back any more. A tick always stands for the same time.
class SessionRunner
{
public:
    /// A tick rate of nought breaks a contract.
    SessionRunner(session::NetworkedSession& session,
                  EventDispatcher& dispatcher,
                  const net::IClock& clock,
                  std::uint16_t tickRate);

    /// Replaces the input the local player plays from the next tick on.
    void setLocalInput(std::span<const std::byte> input);

    /// Lets the given microseconds of the host's time pass.
    [[nodiscard]] RunnerStep update(std::uint64_t hostDeltaMicroseconds);

private:
    void handOverEvents();

    [[nodiscard]] std::uint32_t rollbacksSoFar() const;

    session::NetworkedSession& networked;
    EventDispatcher& dispatcher;
    const net::IClock& clock;
    std::uint16_t tickRate;
    std::uint64_t elapsedTickMicroseconds = 0;
};

}
