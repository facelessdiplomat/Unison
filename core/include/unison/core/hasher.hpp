#pragma once

#include <unison/core/contract.hpp>
#include <unison/core/raw_value.hpp>

#include <xxhash.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace unison
{

/// Streaming XXH3-64 digest: the result depends on the concatenated bytes added and not on how they
/// were split, and equals the one-shot XXH3 over the same stream. A value is hashed as its object
/// bytes, so its padding must already be zeroed by its owner or the digest carries indeterminate bytes.
class Hasher
{
public:
    Hasher()
    {
        [[maybe_unused]] const XXH_errorcode status = XXH3_64bits_reset(&state);
        UNISON_ASSERT(status == XXH_OK);
    }

    void add(std::span<const std::byte> bytes)
    {
        [[maybe_unused]] const XXH_errorcode status = XXH3_64bits_update(&state, bytes.data(), bytes.size());
        UNISON_ASSERT(status == XXH_OK);
    }

    template <RawValue T>
    void add(const T& value)
    {
        add(std::as_bytes(std::span<const T, 1>{&value, 1}));
    }

    [[nodiscard]] std::uint64_t finish() const
    {
        return XXH3_64bits_digest(&state);
    }

private:
    XXH3_state_t state{};
};

}
