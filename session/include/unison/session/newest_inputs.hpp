#pragma once

#include <unison/net/protocol.hpp>
#include <unison/session/session.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace unison::session
{

/// How many of its newest inputs a client sends in every input message, so that a message lost on the way
/// costs nothing as long as the next one arrives.
inline constexpr std::uint32_t kRedundantInputs = 4;

/// The newest inputs of a slot a session holds, as its client sends them to the relay: the newest frame is the
/// predicted one and the input delay on, and the first the later of the one after the verified frame and the one
/// `kRedundantInputs` frames back; nothing when every one of them is verified. The message views the batch it is
/// written into, which must hold `kRedundantInputs` inputs of the size.
[[nodiscard]] std::optional<net::Input> newestInputsOf(const Session& session,
                                                       std::uint8_t slot,
                                                       std::uint32_t inputDelay,
                                                       std::uint8_t inputSize,
                                                       std::span<std::byte> batch);

}
