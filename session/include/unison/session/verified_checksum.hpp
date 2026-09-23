#pragma once

#include <cstdint>

namespace unison::session
{

/// The checksum of a frame the session has verified, for comparing with what every other client made
/// of the same frame.
struct VerifiedChecksum
{
    std::uint32_t frameNumber = 0;
    std::uint64_t checksum = 0;
};

}
