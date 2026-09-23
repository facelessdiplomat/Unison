#include <unison/sim/physics_world.hpp>

#include <unison/core/body_id.hpp>
#include <unison/core/contract.hpp>
#include <unison/core/jolt_conversions.hpp>

#include <Jolt/Physics/StateRecorder.h>

#include <cstring>

namespace unison::sim
{

namespace
{

constexpr JPH::uint kBodyMutexCountForOneThread = 1;
constexpr int kCollisionStepsPerTick = 1;

class StateWriter final : public JPH::StateRecorder
{
public:
    explicit StateWriter(std::vector<std::byte>& bytes) : written{bytes}
    {
    }

    void WriteBytes(const void* data, std::size_t count) override
    {
        const auto* first = static_cast<const std::byte*>(data);

        written.insert(written.end(), first, first + count);
    }

    void ReadBytes(void* data, std::size_t count) override
    {
        std::memset(data, 0, count);
        hasFailed = true;
    }

    [[nodiscard]] bool IsEOF() const override
    {
        return hasFailed;
    }

    [[nodiscard]] bool IsFailed() const override
    {
        return hasFailed;
    }

private:
    std::vector<std::byte>& written;
    bool hasFailed = false;
};

class StateReader final : public JPH::StateRecorder
{
public:
    explicit StateReader(std::span<const std::byte> bytes) : unread{bytes}
    {
    }

    void WriteBytes(const void*, std::size_t) override
    {
        hasFailed = true;
    }

    void ReadBytes(void* data, std::size_t count) override
    {
        if (count > unread.size())
        {
            std::memset(data, 0, count);
            hasFailed = true;

            return;
        }

        std::memcpy(data, unread.data(), count);
        unread = unread.subspan(count);
    }

    [[nodiscard]] bool IsEOF() const override
    {
        return hasFailed;
    }

    [[nodiscard]] bool IsFailed() const override
    {
        return hasFailed;
    }

private:
    std::span<const std::byte> unread;
    bool hasFailed = false;
};

}

PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings& settings)
    : runtime{}, scratchAllocator{settings.scratchBytes}, jobSystem{JPH::cMaxPhysicsJobs}, bodyTable{physicsSystem},
      characterTable{physicsSystem, scratchAllocator}, physicsQueries{physicsSystem}
{
    physicsSystem.Init(kMaxBodies,
                       kBodyMutexCountForOneThread,
                       settings.maxBodyPairs,
                       settings.maxContactConstraints,
                       broadPhaseLayers,
                       objectVsBroadPhaseFilter,
                       objectPairFilter);

    physicsSystem.SetGravity(toJoltVector(settings.gravity));
    physicsSystem.SetContactListener(&contactCollector);
}

void PhysicsWorld::step(float dt)
{
    UNISON_ASSERT(dt > 0.0F);

    contactCollector.clear();

    const JPH::EPhysicsUpdateError error =
        physicsSystem.Update(dt, kCollisionStepsPerTick, &scratchAllocator, &jobSystem);

    contactCollector.sort();

    UNISON_VERIFY(error == JPH::EPhysicsUpdateError::None);
}

BodyTable& PhysicsWorld::bodies()
{
    return bodyTable;
}

const BodyTable& PhysicsWorld::bodies() const
{
    return bodyTable;
}

CharacterTable& PhysicsWorld::characters()
{
    return characterTable;
}

const CharacterTable& PhysicsWorld::characters() const
{
    return characterTable;
}

const PhysicsQueries& PhysicsWorld::queries() const
{
    return physicsQueries;
}

std::span<const Contact> PhysicsWorld::contacts() const
{
    return contactCollector.contacts();
}

void PhysicsWorld::saveState(std::vector<std::byte>& bytes) const
{
    bytes.clear();

    StateWriter recorder{bytes};

    physicsSystem.SaveState(recorder, JPH::EStateRecorderState::All);
    characterTable.saveState(recorder);

    UNISON_VERIFY(!recorder.IsFailed());
}

void PhysicsWorld::restoreState(std::span<const std::byte> bytes)
{
    StateReader recorder{bytes};

    contactCollector.clear();

    const bool restored = physicsSystem.RestoreState(recorder);

    characterTable.restoreState(recorder);

    UNISON_VERIFY(restored);
    UNISON_VERIFY(!recorder.IsFailed());
}

Float3 PhysicsWorld::gravity() const
{
    return toFloat3(physicsSystem.GetGravity());
}

}
