#include <unison/core/asset_id.hpp>
#include <unison/core/binary_reader.hpp>
#include <unison/core/binary_writer.hpp>
#include <unison/core/fixed_string.hpp>
#include <unison/core/fixed_vector.hpp>
#include <unison/core/float3.hpp>
#include <unison/core/fp_env_guard.hpp>
#include <unison/core/hasher.hpp>
#include <unison/core/jolt_conversions.hpp>
#include <unison/core/log_sink.hpp>
#include <unison/core/math.hpp>
#include <unison/core/quaternion.hpp>
#include <unison/core/raw_value.hpp>
#include <unison/core/rng.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace
{

static_assert(unison::RawValue<unison::Float3>);
static_assert(unison::RawValue<unison::Quaternion>);
static_assert(unison::RawValue<unison::Rng>);
static_assert(unison::RawValue<unison::AssetId>);

std::uint64_t checkAssetId()
{
    return static_cast<std::uint64_t>(unison::makeAssetId("floor"));
}

std::uint64_t checkFixedVector()
{
    unison::FixedVector<std::uint32_t, 4> values{};

    values.pushBack(1);
    values.pushBack(2);
    values.popBack();

    std::uint64_t total = values.isFull() ? 1U : 0U;

    for (const std::uint32_t value : values)
    {
        total += value;
    }

    total += values[0];
    total += values.size();
    values.clear();

    return total + values.size() + unison::FixedVector<std::uint32_t, 4>::kCapacity;
}

std::uint64_t checkFixedString()
{
    const unison::FixedString<8> name{std::string_view{"unison"}};
    const unison::FixedString<8> same{std::string_view{"unison"}};

    return name.size() + name.view().size() + (name == same ? 1U : 0U) + unison::FixedString<8>::kCapacity;
}

std::uint64_t checkHasher()
{
    const std::array<std::byte, 4> bytes{};
    unison::Hasher hasher;

    hasher.add(std::as_bytes(std::span{bytes}));
    hasher.add(std::uint32_t{7});

    return hasher.finish();
}

std::uint64_t checkBinaryIo()
{
    std::array<std::byte, 64> storage{};
    unison::BinaryWriter writer{storage};

    if (!writer.writeValue(std::uint32_t{7}) || !writer.writeString("unison") || !writer.writeBytes({}))
    {
        return writer.remaining();
    }

    unison::BinaryReader reader{std::span<const std::byte>{storage}.first(writer.size())};

    const auto value = reader.readValue<std::uint32_t>();
    const auto text = reader.readString();
    const auto bytes = reader.readBytes(0);

    return value.value_or(0U) + (text.has_value() ? text->size() : 0U) + (bytes.has_value() ? bytes->size() : 0U) +
           reader.remaining() + writer.size();
}

std::uint64_t checkFpEnvGuard()
{
    const unison::FpEnvGuard guard;

    return 1U;
}

std::uint64_t checkLogSink()
{
    const unison::LogSink previous = unison::installedLogSink();

    unison::logMessage(unison::LogLevel::Debug, "core header compile check");
    unison::installLogSink(previous);

    return previous == nullptr ? 0U : 1U;
}

std::uint64_t checkMath()
{
    const float angle = unison::math::atan2(1.0F, 2.0F);
    const float blended = unison::math::lerp(unison::math::sin(angle), unison::math::cos(angle), 0.25F);

    return std::bit_cast<std::uint32_t>(unison::math::sqrt(unison::math::clamp(blended, 0.0F, 1.0F)));
}

std::uint64_t checkMathTypes()
{
    const unison::Float3 position{1.0F, 2.0F, 3.0F};
    const unison::Quaternion rotation{};

    const unison::Float3 restoredPosition = unison::toFloat3(unison::toJoltVector(position));
    const unison::Quaternion restoredRotation = unison::toQuaternion(unison::toJoltQuaternion(rotation));

    return std::bit_cast<std::uint32_t>(restoredPosition.x + restoredRotation.w);
}

std::uint64_t checkRng()
{
    unison::Rng rng{42};

    return rng.nextUint32() + std::bit_cast<std::uint32_t>(rng.nextFloat01()) +
           static_cast<std::uint64_t>(rng.nextInRange(0, 7));
}

}

std::uint64_t unisonCoreHeaderCheck()
{
    return checkAssetId() ^ checkBinaryIo() ^ checkFixedString() ^ checkFixedVector() ^ checkFpEnvGuard() ^
           checkHasher() ^ checkLogSink() ^ checkMath() ^ checkMathTypes() ^ checkRng();
}
