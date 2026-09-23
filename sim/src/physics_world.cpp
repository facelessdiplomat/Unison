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

class VectorStateRecorder final : public JPH::StateRecorder
{
public:
    explicit VectorStateRecorder(std::vector<std::byte>& bytes) : recorded{bytes}
    {
    }

    void WriteBytes(const void* data, std::size_t count) override
    {
        const auto* first = static_cast<const std::byte*>(data);

        recorded.insert(recorded.end(), first, first + count);
    }

    void ReadBytes(void* data, std::size_t count) override
    {
        if (readPosition + count > recorded.size())
        {
            std::memset(data, 0, count);
            readPast = true;

            return;
        }

        std::memcpy(data, recorded.data() + readPosition, count);
        readPosition += count;
    }

    [[nodiscard]] bool IsEOF() const override
    {
        return readPast;
    }

    [[nodiscard]] bool IsFailed() const override
    {
        return readPast;
    }

private:
    std::vector<std::byte>& recorded;
    std::size_t readPosition = 0;
    bool readPast = false;
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

    VectorStateRecorder recorder{bytes};

    physicsSystem.SaveState(recorder, JPH::EStateRecorderState::All);
    characterTable.saveState(recorder);

    UNISON_VERIFY(!recorder.IsFailed());
}

void PhysicsWorld::restoreState(std::span<const std::byte> bytes)
{
    std::vector<std::byte> recorded{bytes.begin(), bytes.end()};
    VectorStateRecorder recorder{recorded};

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
