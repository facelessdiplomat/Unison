#include <unison/sim/pipeline_hash.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

namespace
{

class NamedSystem final : public unison::sim::ISystem
{
public:
    explicit NamedSystem(std::string_view systemName) : systemName{systemName}
    {
    }

    void update(unison::sim::Frame&, const unison::sim::FrameInputs&) override
    {
    }

    [[nodiscard]] std::string_view name() const override
    {
        return systemName;
    }

private:
    std::string_view systemName;
};

}

TEST_CASE("pipelines running the same systems in the same order hash alike")
{
    NamedSystem move{"Move"};
    NamedSystem shoot{"Shoot"};

    unison::sim::SystemPipeline one;
    one.add(move);
    one.add(shoot);

    unison::sim::SystemPipeline other;
    other.add(move);
    other.add(shoot);

    REQUIRE(unison::sim::hashOf(one) == unison::sim::hashOf(other));
}

TEST_CASE("running the same systems in another order changes the hash")
{
    NamedSystem move{"Move"};
    NamedSystem shoot{"Shoot"};

    unison::sim::SystemPipeline forward;
    forward.add(move);
    forward.add(shoot);

    unison::sim::SystemPipeline reversed;
    reversed.add(shoot);
    reversed.add(move);

    REQUIRE(unison::sim::hashOf(forward) != unison::sim::hashOf(reversed));
}

TEST_CASE("renaming a system changes the hash")
{
    NamedSystem move{"Move"};
    NamedSystem renamed{"Movement"};

    unison::sim::SystemPipeline before;
    before.add(move);

    unison::sim::SystemPipeline after;
    after.add(renamed);

    REQUIRE(unison::sim::hashOf(before) != unison::sim::hashOf(after));
}

TEST_CASE("adding a system changes the hash")
{
    NamedSystem move{"Move"};
    NamedSystem shoot{"Shoot"};

    unison::sim::SystemPipeline shorter;
    shorter.add(move);

    unison::sim::SystemPipeline longer;
    longer.add(move);
    longer.add(shoot);

    REQUIRE(unison::sim::hashOf(shorter) != unison::sim::hashOf(longer));
}

TEST_CASE("names cannot be run together to reach the same hash")
{
    NamedSystem split{"Move"};
    NamedSystem splitTail{"Shoot"};
    NamedSystem joined{"MoveShoot"};

    unison::sim::SystemPipeline inTwo;
    inTwo.add(split);
    inTwo.add(splitTail);

    unison::sim::SystemPipeline inOne;
    inOne.add(joined);

    REQUIRE(unison::sim::hashOf(inTwo) != unison::sim::hashOf(inOne));
}
