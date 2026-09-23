#pragma once

#include <unison/core/body_id.hpp>
#include <unison/sim/character.hpp>
#include <unison/sim/transform.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <span>
#include <vector>

namespace unison::sim
{

/// The characters walking around one physics world, held in id order so that saving, restoring and
/// stepping them always happen in the same sequence. Jolt keeps a character outside the state its
/// physics system saves, so this table saves and restores them itself.
class CharacterTable
{
public:
    CharacterTable(JPH::PhysicsSystem& system, JPH::TempAllocator& scratch);

    CharacterTable(const CharacterTable&) = delete;
    CharacterTable& operator=(const CharacterTable&) = delete;
    CharacterTable(CharacterTable&&) = delete;
    CharacterTable& operator=(CharacterTable&&) = delete;

    void create(BodyId id, const CharacterController& character, const Transform& placement);

    void destroy(BodyId id);

    [[nodiscard]] bool holds(BodyId id) const;

    /// Moves the character by one tick at the velocity given, sliding along whatever it meets.
    void move(BodyId id, const Float3& velocity, const Float3& gravity, float dt);

    [[nodiscard]] Transform transformOf(BodyId id) const;

    [[nodiscard]] Float3 velocityOf(BodyId id) const;

    [[nodiscard]] GroundState groundOf(BodyId id) const;

    /// Takes out every character the frame does not name: `named` holds, at each body index, the id the
    /// frame names there or `BodyId::Invalid`. Nothing is allocated on the way.
    void keepOnly(std::span<const BodyId, kMaxBodies> named);

    void saveState(JPH::StateRecorder& recorder) const;

    void restoreState(JPH::StateRecorder& recorder);

private:
    struct Entry
    {
        BodyId id = BodyId::Invalid;
        JPH::Ref<JPH::CharacterVirtual> character;
    };

    [[nodiscard]] const JPH::CharacterVirtual* find(BodyId id) const;

    [[nodiscard]] JPH::CharacterVirtual* find(BodyId id);

    JPH::PhysicsSystem& system;
    JPH::TempAllocator& scratch;
    std::vector<Entry> entries;
};

}
