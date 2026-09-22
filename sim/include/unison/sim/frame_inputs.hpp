#pragma once

#include <unison/core/contract.hpp>
#include <unison/sim/padding_free.hpp>

#include <entt/core/type_info.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace unison::sim
{

/// How many players one session simulates.
inline constexpr std::size_t kMaxSlots = 8;

/// How much room one slot gives a game's input. The cap is what lets a slot hold the input as bytes
/// instead of making every type that touches inputs a template.
inline constexpr std::size_t kMaxInputSize = 64;

/// What is known about one slot's input this frame: whether it arrived at all, whether it was
/// guessed while waiting for the real one, and whether it was given up on.
enum class InputFlags : std::uint8_t
{
    None = 0,
    Present = 1U << 0U,
    Predicted = 1U << 1U,
    Dropped = 1U << 2U
};

[[nodiscard]] constexpr InputFlags operator|(InputFlags left, InputFlags right)
{
    return static_cast<InputFlags>(static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right));
}

[[nodiscard]] constexpr InputFlags operator&(InputFlags left, InputFlags right)
{
    return static_cast<InputFlags>(static_cast<std::uint8_t>(left) & static_cast<std::uint8_t>(right));
}

[[nodiscard]] constexpr bool hasFlag(InputFlags flags, InputFlags wanted)
{
    return (flags & wanted) == wanted;
}

/// What a type must be to serve as a game's input: plain data small enough for a slot, and free of
/// padding, because inputs travel over the wire and into replays byte for byte.
template <typename Input>
struct InputTraits
{
    static constexpr bool isValid =
        std::is_trivially_copyable_v<Input> && sizeof(Input) <= kMaxInputSize && PaddingFree<Input>;
};

/// One input per player slot for one frame, held as bytes so that nothing which passes inputs around
/// has to become a template. A game has one input type; reading a slot as any other breaks a
/// contract. A slot never written reads back neutral and absent.
class FrameInputs
{
public:
    template <typename Input>
    void set(std::size_t slot, const Input& input, InputFlags flags)
    {
        static_assert(InputTraits<Input>::isValid, "an input must be plain data of at most 64 bytes");

        UNISON_VERIFY(slot < kMaxSlots);

        rememberInputType(entt::type_hash<Input>::value());
        std::memcpy(slots.at(slot).data(), &input, sizeof(Input));
        slotFlags.at(slot) = flags;
    }

    template <typename Input>
    [[nodiscard]] Input get(std::size_t slot) const
    {
        static_assert(InputTraits<Input>::isValid, "an input must be plain data of at most 64 bytes");

        UNISON_VERIFY(slot < kMaxSlots);
        UNISON_VERIFY(inputTypeId == 0 || inputTypeId == entt::type_hash<Input>::value());

        Input input{};
        std::memcpy(&input, slots.at(slot).data(), sizeof(Input));

        return input;
    }

    [[nodiscard]] InputFlags flagsAt(std::size_t slot) const;

    void setFlags(std::size_t slot, InputFlags flags);

private:
    void rememberInputType(entt::id_type typeId);

    std::array<std::array<std::byte, kMaxInputSize>, kMaxSlots> slots{};
    std::array<InputFlags, kMaxSlots> slotFlags{};
    entt::id_type inputTypeId = 0;
};

}
