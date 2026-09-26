#include <unison/session/verified_frame_fan_out.hpp>

#include <unison/session/verified_frame_receiver.hpp>
#include <unison/sim/frame_inputs.hpp>
#include <unison/sim/frame_snapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace
{

class FrameNumbers final : public unison::session::IVerifiedFrameReceiver
{
public:
    void frameVerified(const unison::session::VerifiedFrame& frame) override
    {
        heardFrames.push_back(frame.frameNumber);
    }

    [[nodiscard]] const std::vector<std::uint32_t>& heard() const
    {
        return heardFrames;
    }

private:
    std::vector<std::uint32_t> heardFrames;
};

}

TEST_CASE("a fan-out hands every verified frame to each of its receivers")
{
    FrameNumbers first;
    FrameNumbers second;
    unison::session::VerifiedFrameFanOut fanOut{{&first, &second}};
    const unison::sim::FrameInputs inputs;
    const unison::sim::FrameSnapshot snapshot;

    fanOut.frameVerified(unison::session::VerifiedFrame{5, inputs, std::nullopt, snapshot});

    REQUIRE(first.heard() == std::vector<std::uint32_t>{5});
    REQUIRE(second.heard() == std::vector<std::uint32_t>{5});
}
