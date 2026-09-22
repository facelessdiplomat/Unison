#pragma once

#include <unison/sim/character.hpp>
#include <unison/sim/transform.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Character/CharacterVirtual.h>

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

    /// Names every character in the table, in id order.
    void collect(std::vector<BodyId>& characters) const;

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
