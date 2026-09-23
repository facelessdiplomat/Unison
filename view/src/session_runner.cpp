#include <unison/view/session_runner.hpp>

#include <unison/core/contract.hpp>

namespace unison::view
{

namespace
{

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000;

}

SessionRunner::SessionRunner(session::NetworkedSession& session,
                             EventDispatcher& dispatcher,
                             const net::IClock& clock,
                             std::uint16_t tickRate)
    : networked{session}, dispatcher{dispatcher}, clock{clock}, tickRate{tickRate},
      lastUpdatedAt{clock.nowMicroseconds()}
{
    UNISON_VERIFY(tickRate > 0);
}

SessionRunner::SessionRunner(session::NetworkedSession& session, EventDispatcher& dispatcher, std::uint16_t tickRate)
    : networked{session}, dispatcher{dispatcher}, ownClock{std::in_place}, clock{*ownClock}, tickRate{tickRate},
      lastUpdatedAt{clock.nowMicroseconds()}
{
    UNISON_VERIFY(tickRate > 0);
}

void SessionRunner::setLocalInput(std::span<const std::byte> input)
{
    networked.setLocalInput(input);
}

RunnerStep SessionRunner::update(std::uint64_t hostDeltaMicroseconds)
{
    networked.update(clock.nowMicroseconds());

    elapsedTickMicroseconds += hostDeltaMicroseconds * tickRate;

    auto ticks = static_cast<std::uint32_t>(elapsedTickMicroseconds / kMicrosecondsPerSecond);
    elapsedTickMicroseconds %= kMicrosecondsPerSecond;

    if (ticks > 0)
    {
        ticks = static_cast<std::uint32_t>(static_cast<std::int64_t>(ticks) + networked.takeTickCorrection());
    }

    const std::uint32_t rollbacksBefore = rollbacksSoFar();

    for (std::uint32_t tick = 0; tick < ticks; ++tick)
    {
        networked.tick();
    }

    handOverEvents();

    return RunnerStep{ticks,
                      rollbacksSoFar() - rollbacksBefore,
                      static_cast<float>(elapsedTickMicroseconds) / static_cast<float>(kMicrosecondsPerSecond)};
}

RunnerStep SessionRunner::update()
{
    const std::uint64_t now = clock.nowMicroseconds();
    const std::uint64_t hostDelta = now - lastUpdatedAt;

    lastUpdatedAt = now;

    return update(hostDelta);
}

void SessionRunner::handOverEvents()
{
    const session::Session* played = networked.session();

    if (played == nullptr)
    {
        return;
    }

    dispatcher.dispatch(played->eventChanges());
    networked.clearEventChanges();
    dispatcher.forgetBelow(played->verifiedFrame());
}

std::uint32_t SessionRunner::rollbacksSoFar() const
{
    const session::Session* played = networked.session();

    return played == nullptr ? 0U : played->rollbackStats().rollbacks;
}

}
