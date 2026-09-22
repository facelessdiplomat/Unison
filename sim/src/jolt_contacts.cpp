#include <unison/sim/jolt_contacts.hpp>

#include <unison/core/jolt_conversions.hpp>

#include <Jolt/Physics/Body/Body.h>

#include <algorithm>
#include <tuple>

namespace unison::sim
{

void ContactCollector::OnContactAdded(const JPH::Body& first,
                                      const JPH::Body& second,
                                      const JPH::ContactManifold& manifold,
                                      JPH::ContactSettings&)
{
    collect(first, second, manifold, ContactPhase::Began);
}

void ContactCollector::OnContactPersisted(const JPH::Body& first,
                                          const JPH::Body& second,
                                          const JPH::ContactManifold& manifold,
                                          JPH::ContactSettings&)
{
    collect(first, second, manifold, ContactPhase::Continued);
}

void ContactCollector::clear()
{
    collected.clear();
    sorted.clear();
}

void ContactCollector::sort()
{
    const auto byBodiesThenSubShapes = [](const CollectedContact& first, const CollectedContact& second)
    {
        return std::tie(first.contact.first, first.contact.second, first.firstSubShape, first.secondSubShape) <
               std::tie(second.contact.first, second.contact.second, second.firstSubShape, second.secondSubShape);
    };

    std::sort(collected.begin(), collected.end(), byBodiesThenSubShapes);

    sorted.reserve(collected.size());

    for (const CollectedContact& contact : collected)
    {
        sorted.push_back(contact.contact);
    }
}

std::span<const Contact> ContactCollector::contacts() const
{
    return std::span<const Contact>{sorted};
}

void ContactCollector::collect(const JPH::Body& first,
                               const JPH::Body& second,
                               const JPH::ContactManifold& manifold,
                               ContactPhase phase)
{
    const BodyId one = toBodyId(first.GetID());
    const BodyId other = toBodyId(second.GetID());
    const bool inOrder = one < other;

    collected.push_back(
        CollectedContact{Contact{inOrder ? one : other, inOrder ? other : one, phase},
                         inOrder ? manifold.mSubShapeID1.GetValue() : manifold.mSubShapeID2.GetValue(),
                         inOrder ? manifold.mSubShapeID2.GetValue() : manifold.mSubShapeID1.GetValue()});
}

}
