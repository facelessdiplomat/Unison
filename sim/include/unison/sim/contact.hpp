#pragma once

#include <unison/core/body_id.hpp>

#include <cstdint>

namespace unison::sim
{

/// Whether a pair of bodies met during this tick or was already touching when it began.
enum class ContactPhase : std::uint8_t
{
    Began,
    Continued
};

/// Two bodies that touched during a tick, the lower id first, so a pair reads the same on every
/// client whichever way round Jolt happened to report it.
struct Contact
{
    BodyId first = BodyId::Invalid;
    BodyId second = BodyId::Invalid;
    ContactPhase phase = ContactPhase::Began;
};

}
