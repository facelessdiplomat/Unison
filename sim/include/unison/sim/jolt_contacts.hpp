#pragma once

#include <unison/sim/contact.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Collision/ContactListener.h>

#include <cstdint>
#include <span>
#include <vector>

namespace unison::sim
{

/// Collects the contacts Jolt reports while it steps and holds them until the step is over. Jolt
/// reports them in the order its solver reaches them, so they are sorted before anyone reads them
/// and the list is emptied at the start of every step.
class ContactCollector final : public JPH::ContactListener
{
public:
    void OnContactAdded(const JPH::Body& first,
                        const JPH::Body& second,
                        const JPH::ContactManifold& manifold,
                        JPH::ContactSettings& settings) override;

    void OnContactPersisted(const JPH::Body& first,
                            const JPH::Body& second,
                            const JPH::ContactManifold& manifold,
                            JPH::ContactSettings& settings) override;

    /// Forgets the contacts collected so far, because they no longer describe the world.
    void clear();

    /// Puts what was collected into the order every client reads it in.
    void sort();

    [[nodiscard]] std::span<const Contact> contacts() const;

private:
    struct CollectedContact
    {
        Contact contact;
        std::uint32_t firstSubShape = 0;
        std::uint32_t secondSubShape = 0;
    };

    void
    collect(const JPH::Body& first, const JPH::Body& second, const JPH::ContactManifold& manifold, ContactPhase phase);

    std::vector<CollectedContact> collected;
    std::vector<Contact> sorted;
};

}
