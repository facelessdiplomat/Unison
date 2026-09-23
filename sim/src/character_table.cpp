#include <unison/sim/character_table.hpp>

#include <unison/core/contract.hpp>
#include <unison/core/jolt_conversions.hpp>
#include <unison/sim/jolt_layers.hpp>

#include <Jolt/Physics/PhysicsSystem.h>

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <algorithm>
#include <cstdint>
#include <utility>

namespace unison::sim
{

namespace
{

GroundState toGroundState(JPH::CharacterBase::EGroundState ground)
{
    switch (ground)
    {
        case JPH::CharacterBase::EGroundState::OnGround:
            return GroundState::OnGround;
        case JPH::CharacterBase::EGroundState::OnSteepGround:
            return GroundState::OnSteepGround;
        case JPH::CharacterBase::EGroundState::NotSupported:
            return GroundState::NotSupported;
        case JPH::CharacterBase::EGroundState::InAir:
            break;
    }

    return GroundState::InAir;
}

JPH::Ref<JPH::Shape> standingShapeOf(const CharacterController& character)
{
    JPH::CapsuleShapeSettings capsule{character.halfHeight, character.radius};
    capsule.SetEmbedded();

    JPH::RotatedTranslatedShapeSettings standing{
        JPH::Vec3{0.0F, character.halfHeight + character.radius, 0.0F}, JPH::Quat::sIdentity(), &capsule};
    standing.SetEmbedded();

    const JPH::ShapeSettings::ShapeResult result = standing.Create();

    UNISON_VERIFY(result.IsValid());

    return result.IsValid() ? result.Get() : JPH::Ref<JPH::Shape>{};
}

}

CharacterTable::CharacterTable(JPH::PhysicsSystem& system, JPH::TempAllocator& scratch)
    : system{system}, scratch{scratch}
{
}

void CharacterTable::create(BodyId id, const CharacterController& character, const Transform& placement)
{
    UNISON_VERIFY(!holds(id));

    JPH::CharacterVirtualSettings settings;
    settings.SetEmbedded();
    settings.mID = JPH::CharacterID{static_cast<std::uint32_t>(id)};
    settings.mShape = standingShapeOf(character);
    settings.mUp = JPH::Vec3::sAxisY();

    Entry entry{id,
                JPH::Ref<JPH::CharacterVirtual>{new JPH::CharacterVirtual{
                    &settings, toJoltVector(placement.position), toJoltQuaternion(placement.rotation), &system}}};

    entry.character->SetLinearVelocity(toJoltVector(character.velocity));

    const auto place = std::lower_bound(
        entries.begin(), entries.end(), id, [](const Entry& entry, BodyId wanted) { return entry.id < wanted; });

    entries.insert(place, std::move(entry));
}

void CharacterTable::destroy(BodyId id)
{
    const auto found =
        std::find_if(entries.begin(), entries.end(), [id](const Entry& entry) { return entry.id == id; });

    UNISON_VERIFY(found != entries.end());

    if (found != entries.end())
    {
        entries.erase(found);
    }
}

bool CharacterTable::holds(BodyId id) const
{
    return find(id) != nullptr;
}

void CharacterTable::move(BodyId id, const Float3& velocity, const Float3& gravity, float dt)
{
    JPH::CharacterVirtual* character = find(id);

    UNISON_VERIFY(character != nullptr);

    if (character == nullptr)
    {
        return;
    }

    character->SetLinearVelocity(toJoltVector(velocity));
    character->ExtendedUpdate(dt,
                              toJoltVector(gravity),
                              JPH::CharacterVirtual::ExtendedUpdateSettings{},
                              system.GetDefaultBroadPhaseLayerFilter(toObjectLayer(PhysicsLayer::Moving)),
                              system.GetDefaultLayerFilter(toObjectLayer(PhysicsLayer::Moving)),
                              JPH::BodyFilter{},
                              JPH::ShapeFilter{},
                              scratch);
}

Transform CharacterTable::transformOf(BodyId id) const
{
    const JPH::CharacterVirtual* character = find(id);

    UNISON_VERIFY(character != nullptr);

    if (character == nullptr)
    {
        return Transform{};
    }

    return Transform{toFloat3(character->GetPosition()), toQuaternion(character->GetRotation())};
}

Float3 CharacterTable::velocityOf(BodyId id) const
{
    const JPH::CharacterVirtual* character = find(id);

    UNISON_VERIFY(character != nullptr);

    return character == nullptr ? Float3{} : toFloat3(character->GetLinearVelocity());
}

GroundState CharacterTable::groundOf(BodyId id) const
{
    const JPH::CharacterVirtual* character = find(id);

    UNISON_VERIFY(character != nullptr);

    return character == nullptr ? GroundState::InAir : toGroundState(character->GetGroundState());
}

void CharacterTable::keepOnly(std::span<const BodyId, kMaxBodies> named)
{
    std::erase_if(entries, [named](const Entry& entry) { return named[bodyIndexOf(entry.id)] != entry.id; });
}

void CharacterTable::saveState(JPH::StateRecorder& recorder) const
{
    for (const Entry& entry : entries)
    {
        entry.character->SaveState(recorder);
    }
}

void CharacterTable::restoreState(JPH::StateRecorder& recorder)
{
    for (const Entry& entry : entries)
    {
        entry.character->RestoreState(recorder);
    }
}

const JPH::CharacterVirtual* CharacterTable::find(BodyId id) const
{
    for (const Entry& entry : entries)
    {
        if (entry.id == id)
        {
            return entry.character.GetPtr();
        }
    }

    return nullptr;
}

JPH::CharacterVirtual* CharacterTable::find(BodyId id)
{
    return const_cast<JPH::CharacterVirtual*>(std::as_const(*this).find(id));
}

}
