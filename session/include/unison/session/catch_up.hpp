#pragma once

#include <cstdint>
#include <optional>

namespace unison::session
{

/// How many ticks a late joiner adds to a host frame at most while it catches up, so it plays eight frames in one.
inline constexpr std::int32_t kCatchUpExtraTicks = 7;

/// How a client that started from a snapshot catches up with the newest frame the relay has confirmed: behind until it
/// has played that frame, and until then asking for up to `kCatchUpExtraTicks` extra ticks a host frame.
class CatchUp
{
public:
    /// Notes a frame the relay confirmed; the newest heard is the one to catch up with.
    void hear(std::uint32_t confirmedFrame);

    /// Starts catching up, as a client does once it has restored a snapshot.
    void start();

    /// Stops catching up, for good, once the client has played the newest frame it heard confirmed.
    void update(std::uint32_t predictedFrame);

    [[nodiscard]] bool isBehind() const;

    /// The extra ticks a host frame adds while the client is behind; nothing once it has caught up.
    [[nodiscard]] std::optional<std::int32_t> extraTicks(std::uint32_t predictedFrame) const;

    /// The newest frame heard confirmed, nought before any.
    [[nodiscard]] std::uint32_t newestHeard() const;

private:
    std::uint32_t newestConfirmed = 0;
    bool isCatchingUp = false;
};

/// The ticks a spectator adds to or takes from a host frame to play, as soon as it may, every frame `delayFrames`
/// behind the newest the relay has confirmed: one fewer when it may play none, and one more for every frame after the
/// first it may play, up to `kCatchUpExtraTicks`.
[[nodiscard]] std::int32_t
spectatorTickCorrection(std::uint32_t newestConfirmed, std::uint32_t delayFrames, std::uint32_t predictedFrame);

}
