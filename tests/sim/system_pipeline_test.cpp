#include <unison/sim/system_pipeline.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace
{

class RecordingSystem final : public unison::sim::ISystem
{
public:
    RecordingSystem(std::string_view systemName, std::vector<std::string>& log) : systemName{systemName}, log{log}
    {
    }

    void update(unison::sim::Frame& frame, const unison::sim::FrameInputs&) override
    {
        log.push_back(std::string{systemName});
        ++frame.frameNumber;
    }

    [[nodiscard]] std::string_view name() const override
    {
        return systemName;
    }

private:
    std::string_view systemName;
    std::vector<std::string>& log;
};

}

TEST_CASE("a pipeline runs its systems in the order they were added")
{
    std::vector<std::string> log;
    RecordingSystem first{"first", log};
    RecordingSystem second{"second", log};
    RecordingSystem third{"third", log};

    unison::sim::SystemPipeline pipeline;
    pipeline.add(first);
    pipeline.add(second);
    pipeline.add(third);

    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    pipeline.update(frame, inputs);

    REQUIRE(log == std::vector<std::string>{"first", "second", "third"});
}

TEST_CASE("a pipeline runs each system exactly once")
{
    std::vector<std::string> log;
    RecordingSystem only{"only", log};

    unison::sim::SystemPipeline pipeline;
    pipeline.add(only);

    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    pipeline.update(frame, inputs);

    REQUIRE(log.size() == 1U);
    REQUIRE(frame.frameNumber == 1U);
}

TEST_CASE("an empty pipeline leaves the frame alone")
{
    const unison::sim::SystemPipeline pipeline;
    unison::sim::Frame frame;
    const unison::sim::FrameInputs inputs;

    pipeline.update(frame, inputs);

    REQUIRE(pipeline.size() == 0U);
    REQUIRE(frame.frameNumber == 0U);
}

TEST_CASE("a pipeline remembers its systems by name and in order")
{
    std::vector<std::string> log;
    RecordingSystem first{"first", log};
    RecordingSystem second{"second", log};

    unison::sim::SystemPipeline pipeline;
    pipeline.add(first);
    pipeline.add(second);

    REQUIRE(pipeline.size() == 2U);
    REQUIRE(pipeline.at(0).name() == "first");
    REQUIRE(pipeline.at(1).name() == "second");
}
