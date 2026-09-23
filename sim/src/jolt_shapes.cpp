#include <unison/sim/jolt_shapes.hpp>

#include <unison/core/contract.hpp>
#include <unison/core/jolt_conversions.hpp>

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

namespace unison::sim
{

namespace
{

JPH::Ref<JPH::Shape> createdShape(const JPH::ShapeSettings& settings)
{
    const JPH::ShapeSettings::ShapeResult result = settings.Create();

    UNISON_VERIFY(result.IsValid());

    return result.IsValid() ? result.Get() : JPH::Ref<JPH::Shape>{};
}

}

JPH::Ref<JPH::Shape> shapeOf(const BodyDefinition& definition)
{
    switch (definition.shape)
    {
        case BodyShape::Sphere:
            return sphereShape(definition.radius);
        case BodyShape::Capsule:
            return capsuleShape(definition.halfHeight, definition.radius);
        case BodyShape::Box:
            break;
    }

    JPH::BoxShapeSettings box{toJoltVector(definition.halfExtents)};
    box.SetEmbedded();

    return createdShape(box);
}

JPH::Ref<JPH::Shape> sphereShape(float radius)
{
    JPH::SphereShapeSettings sphere{radius};
    sphere.SetEmbedded();

    return createdShape(sphere);
}

JPH::Ref<JPH::Shape> capsuleShape(float halfHeight, float radius)
{
    JPH::CapsuleShapeSettings capsule{halfHeight, radius};
    capsule.SetEmbedded();

    return createdShape(capsule);
}

}
