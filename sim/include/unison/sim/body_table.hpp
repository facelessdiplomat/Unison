#pragma once

#include <unison/core/body_id.hpp>
#include <unison/sim/body_definition.hpp>
#include <unison/sim/transform.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/PhysicsSystem.h>

#include <cstdint>
#include <vector>

namespace unison::sim
{

/// The bodies of one physics world, each under the id its frame handed out for it. It builds and takes
/// down bodies and answers what Jolt knows about each of them; the world they live in steps them.
class BodyTable
{
public:
    explicit BodyTable(JPH::PhysicsSystem& system);

    BodyTable(const BodyTable&) = delete;
    BodyTable& operator=(const BodyTable&) = delete;
    BodyTable(BodyTable&&) = delete;
    BodyTable& operator=(BodyTable&&) = delete;

    /// Builds the body a definition describes, placed where the transform says, under the id the
    /// frame handed out. No body of this world may carry that id yet.
    void create(BodyId id, const BodyDefinition& definition, const Transform& placement);

    /// Takes a body of this world out of it, leaving its id free to be handed out again.
    void destroy(BodyId id);

    [[nodiscard]] bool holds(BodyId id) const;

    [[nodiscard]] Transform transformOf(BodyId id) const;

    /// Puts back on a body what Jolt leaves out of the state buffer it saves.
    void applyProperties(BodyId id, const BodyDefinition& definition);

    [[nodiscard]] float frictionOf(BodyId id) const;

    [[nodiscard]] float restitutionOf(BodyId id) const;

    /// What the body weighs in kilogrammes, or nothing at all when it is not a body forces move.
    [[nodiscard]] float massOf(BodyId id) const;

    [[nodiscard]] BodyMotion motionOf(BodyId id) const;

    /// Names every body the world holds, in the order Jolt keeps them.
    void collect(std::vector<BodyId>& bodies) const;

    [[nodiscard]] std::uint32_t count() const;

private:
    JPH::PhysicsSystem& system;
};

}
