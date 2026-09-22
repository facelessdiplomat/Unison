#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>

namespace unison
{

/// xoshiro256** pseudo random generator, the only source of randomness the simulation may use. Its
/// whole state is four integers, so it travels inside a snapshot as plain bytes, and every draw is
/// integer arithmetic, so two machines running the same seed produce the same stream.
class Rng
{
public:
    constexpr Rng() : Rng{0}
    {
    }

    constexpr explicit Rng(std::uint64_t seed)
    {
        std::uint64_t expansion = seed;

        for (std::uint64_t& word : state)
        {
            word = splitMix64(expansion);
        }
    }

    /// Uniform 32-bit draw, taken from the high half of one 64-bit step of the generator.
    [[nodiscard]] constexpr std::uint32_t nextUint32()
    {
        return static_cast<std::uint32_t>(nextUint64() >> 32U);
    }

    /// Uniform draw in the half open interval from zero to one, built from 24 bits so the scaling is
    /// an exact power of two and cannot round.
    [[nodiscard]] constexpr float nextFloat01()
    {
        constexpr float kMantissaScale = 0x1.0p-24F;

        return static_cast<float>(nextUint64() >> 40U) * kMantissaScale;
    }

    /// Uniform draw from the closed range, rejecting the draws that would favour its lower values.
    /// A range whose minimum exceeds its maximum is a contract violation.
    [[nodiscard]] constexpr std::int32_t nextInRange(std::int32_t minimum, std::int32_t maximum)
    {
        assert(minimum <= maximum);

        const std::uint64_t span = static_cast<std::uint64_t>(static_cast<std::int64_t>(maximum) - minimum) + 1U;
        const std::uint64_t bucketSize = std::numeric_limits<std::uint64_t>::max() / span;
        const std::uint64_t limit = bucketSize * span;

        std::uint64_t draw = nextUint64();

        while (draw >= limit)
        {
            draw = nextUint64();
        }

        return static_cast<std::int32_t>(static_cast<std::int64_t>(minimum) +
                                         static_cast<std::int64_t>(draw / bucketSize));
    }

private:
    static constexpr std::uint64_t rotateLeft(std::uint64_t value, int bits)
    {
        assert(bits > 0 && bits < 64);

        return (value << bits) | (value >> (64 - bits));
    }

    static constexpr std::uint64_t splitMix64(std::uint64_t& expansion)
    {
        expansion += 0x9E3779B97F4A7C15U;

        std::uint64_t mixed = expansion;
        mixed = (mixed ^ (mixed >> 30U)) * 0xBF58476D1CE4E5B9U;
        mixed = (mixed ^ (mixed >> 27U)) * 0x94D049BB133111EBU;

        return mixed ^ (mixed >> 31U);
    }

    constexpr std::uint64_t nextUint64()
    {
        const std::uint64_t result = rotateLeft(state[1] * 5U, 7) * 9U;
        const std::uint64_t shifted = state[1] << 17U;

        state[2] ^= state[0];
        state[3] ^= state[1];
        state[1] ^= state[2];
        state[0] ^= state[3];
        state[2] ^= shifted;
        state[3] = rotateLeft(state[3], 45);

        return result;
    }

    std::array<std::uint64_t, 4> state{};
};

}
