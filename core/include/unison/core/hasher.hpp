#pragma once

#include <xxhash.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace unison
{

namespace detail
{

template <typename T>
inline constexpr bool kIsSpan = false;

template <typename T, std::size_t Extent>
inline constexpr bool kIsSpan<std::span<T, Extent>> = true;

}

/// Streaming XXH3-64 digest: the result depends on the concatenated bytes added and not on how they
/// were split, and equals the one-shot XXH3 over the same stream. A value is hashed as its object
/// bytes, so its padding must already be zeroed by its owner or the digest carries indeterminate bytes.
class Hasher
{
public:
    Hasher()
    {
        [[maybe_unused]] const XXH_errorcode status = XXH3_64bits_reset(&state);
        assert(status == XXH_OK);
    }

    void add(std::span<const std::byte> bytes)
    {
        [[maybe_unused]] const XXH_errorcode status = XXH3_64bits_update(&state, bytes.data(), bytes.size());
        assert(status == XXH_OK);
    }

    template <typename T>
        requires(!detail::kIsSpan<T>)
    void add(const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Hasher takes trivially copyable values only");
        static_assert(!std::is_pointer_v<T>, "hashing a pointer would hash an address, which differs per process");

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
