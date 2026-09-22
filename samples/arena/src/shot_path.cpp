#include <arena/shot_path.hpp>

#include <unison/core/math.hpp>

namespace arena
{

namespace
{

constexpr float kTooShortToPointAnywhere = 1.0e-8F;

unison::Float3 difference(const unison::Float3& from, const unison::Float3& to)
{
    return unison::Float3{to.x - from.x, to.y - from.y, to.z - from.z};
}

float dot(const unison::Float3& first, const unison::Float3& second)
{
    return first.x * second.x + first.y * second.y + first.z * second.z;
}

unison::Float3 along(const unison::Float3& from, const unison::Float3& direction, float fraction)
{
    return unison::Float3{
        from.x + direction.x * fraction, from.y + direction.y * fraction, from.z + direction.z * fraction};
}

float squaredDistance(const unison::Float3& first, const unison::Float3& second)
{
    const unison::Float3 apart = difference(first, second);

    return dot(apart, apart);
}

}

ShotApproach sweepPastCapsule(const unison::Float3& from,
                              const unison::Float3& to,
                              float shotRadius,
                              const unison::Float3& capsuleBottom,
                              const unison::Float3& capsuleTop,
                              float capsuleRadius)
{
    const unison::Float3 path = difference(from, to);
    const unison::Float3 axis = difference(capsuleBottom, capsuleTop);
    const unison::Float3 offset = difference(capsuleBottom, from);

    const float pathLengthSquared = dot(path, path);
    const float axisLengthSquared = dot(axis, axis);
    const float axisTowardsShot = dot(axis, offset);

    float onPath = 0.0F;
    float onAxis = 0.0F;

    if (pathLengthSquared < kTooShortToPointAnywhere)
    {
        onAxis = axisLengthSquared < kTooShortToPointAnywhere
                     ? 0.0F
                     : unison::math::clamp(axisTowardsShot / axisLengthSquared, 0.0F, 1.0F);
    }
    else
    {
        const float pathTowardsShot = dot(path, offset);

        if (axisLengthSquared < kTooShortToPointAnywhere)
        {
            onPath = unison::math::clamp(-pathTowardsShot / pathLengthSquared, 0.0F, 1.0F);
        }
        else
        {
            const float pathAlongAxis = dot(path, axis);
            const float spread = pathLengthSquared * axisLengthSquared - pathAlongAxis * pathAlongAxis;

            onPath =
                spread < kTooShortToPointAnywhere
                    ? 0.0F
                    : unison::math::clamp(
                          (pathAlongAxis * axisTowardsShot - pathTowardsShot * axisLengthSquared) / spread, 0.0F, 1.0F);

            onAxis = (pathAlongAxis * onPath + axisTowardsShot) / axisLengthSquared;

            if (onAxis < 0.0F)
            {
                onAxis = 0.0F;
                onPath = unison::math::clamp(-pathTowardsShot / pathLengthSquared, 0.0F, 1.0F);
            }
            else if (onAxis > 1.0F)
            {
                onAxis = 1.0F;
                onPath = unison::math::clamp((pathAlongAxis - pathTowardsShot) / pathLengthSquared, 0.0F, 1.0F);
            }
        }
    }

    const float reach = shotRadius + capsuleRadius;
    const float apart = squaredDistance(along(from, path, onPath), along(capsuleBottom, axis, onAxis));

    return ShotApproach{onPath, apart <= reach * reach};
}

}
