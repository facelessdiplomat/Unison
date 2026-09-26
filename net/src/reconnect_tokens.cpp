#include <unison/net/reconnect_tokens.hpp>

namespace unison::net
{

ReconnectTokens::ReconnectTokens(std::uint64_t seed) : rng{seed}
{
}

std::uint64_t ReconnectTokens::next()
{
    const std::uint64_t high = rng.nextUint32();
    const std::uint64_t low = rng.nextUint32();

    return (high << 32U) | low | 1U;
}

}
