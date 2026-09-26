#include <unison/session/confirmed_inputs.hpp>

#include <unison/sim/frame_inputs.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

TEST_CASE("the slots of a confirmed frame become the flags and the inputs a frame plays")
{
    constexpr std::uint8_t kSlots = 3;
    constexpr std::uint8_t kInputSize = 2;
    const std::array<std::byte, 9> slots{std::byte{1},
                                         std::byte{4},
                                         std::byte{5},
                                         std::byte{2},
                                         std::byte{6},
                                         std::byte{7},
                                         std::byte{0},
                                         std::byte{0},
                                         std::byte{0}};

    const unison::sim::FrameInputs inputs = unison::session::inputsOfConfirmedFrame(slots, kSlots, kInputSize);

    REQUIRE(inputs.flagsAt(0) == unison::sim::InputFlags::Present);
    REQUIRE(inputs.flagsAt(1) == unison::sim::InputFlags::Dropped);
    REQUIRE(inputs.flagsAt(2) == unison::sim::InputFlags::None);
    REQUIRE(inputs.bytesAt(0)[0] == std::byte{4});
    REQUIRE(inputs.bytesAt(0)[1] == std::byte{5});
    REQUIRE(inputs.bytesAt(1)[0] == std::byte{6});
    REQUIRE(inputs.bytesAt(1)[1] == std::byte{7});
}
