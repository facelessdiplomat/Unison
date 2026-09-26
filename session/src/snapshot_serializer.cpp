#include <unison/session/snapshot_serializer.hpp>

#include <unison/core/binary_reader.hpp>
#include <unison/core/binary_writer.hpp>
#include <unison/core/body_id.hpp>
#include <unison/core/contract.hpp>
#include <unison/core/rng.hpp>
#include <unison/sim/body_id_allocator.hpp>
#include <unison/sim/component_registry.hpp>
#include <unison/sim/globals.hpp>
#include <unison/sim/registry_serialization.hpp>

#include <array>
#include <limits>
#include <optional>
#include <string_view>

namespace unison::session
{

namespace
{

constexpr std::size_t kHeaderSize = sizeof(kSnapshotMagic) + sizeof(kSnapshotVersion) + sizeof(std::uint64_t);

tl::unexpected<Error> malformed(std::string_view reason)
{
    return tl::unexpected{Error{ErrorCode::MalformedSnapshot, reason}};
}

std::size_t serializedSizeOf(const sim::Globals& globals)
{
    return sizeof(Rng) + sizeof(sim::MatchPhase) + sizeof(std::uint32_t) +
           globals.bodyIds.freeIds().size() * sizeof(BodyId) + sizeof(std::uint32_t);
}

bool writeGlobals(BinaryWriter& writer, const sim::Globals& globals)
{
    const std::span<const BodyId> freeIds = globals.bodyIds.freeIds();
    bool isWritten = writer.writeValue(globals.rng) && writer.writeValue(globals.matchPhase) &&
                     writer.writeValue(static_cast<std::uint32_t>(freeIds.size()));

    for (const BodyId id : freeIds)
    {
        isWritten = isWritten && writer.writeValue(id);
    }

    return isWritten && writer.writeValue(globals.bodyIds.takenSlotCount());
}

std::optional<sim::BodyIdAllocator> readBodyIds(BinaryReader& reader)
{
    const std::optional<std::uint32_t> freeCount = reader.readValue<std::uint32_t>();

    if (!freeCount.has_value() || *freeCount > kMaxBodies)
    {
        return std::nullopt;
    }

    std::array<BodyId, kMaxBodies> freeIds{};

    for (std::uint32_t read = 0; read < *freeCount; ++read)
    {
        const std::optional<BodyId> id = reader.readValue<BodyId>();

        if (!id.has_value())
        {
            return std::nullopt;
        }

        freeIds.at(read) = *id;
    }

    const std::optional<std::uint32_t> takenSlots = reader.readValue<std::uint32_t>();

    return takenSlots.has_value() ? sim::BodyIdAllocator::fromState(std::span{freeIds}.first(*freeCount), *takenSlots)
                                  : std::nullopt;
}

std::optional<sim::Globals> readGlobals(BinaryReader& reader)
{
    const std::optional<Rng> rng = reader.readValue<Rng>();
    const std::optional<sim::MatchPhase> matchPhase =
        rng.has_value() ? reader.readValue<sim::MatchPhase>() : std::nullopt;

    if (!matchPhase.has_value() || *matchPhase > sim::MatchPhase::Ended)
    {
        return std::nullopt;
    }

    const std::optional<sim::BodyIdAllocator> bodyIds = readBodyIds(reader);

    if (!bodyIds.has_value())
    {
        return std::nullopt;
    }

    return sim::Globals{*rng, *matchPhase, *bodyIds};
}

tl::expected<void, Error> readHeader(BinaryReader& reader)
{
    const std::optional<std::uint32_t> magic = reader.readValue<std::uint32_t>();
    const std::optional<std::uint16_t> version =
        magic == kSnapshotMagic ? reader.readValue<std::uint16_t>() : std::nullopt;

    if (version != kSnapshotVersion)
    {
        return malformed("the bytes are no snapshot of this version of the format");
    }

    if (reader.readValue<std::uint64_t>() != sim::componentRegistry().layoutHash())
    {
        return malformed("the snapshot was written with another layout of components than this build's");
    }

    return {};
}

}

void serializeSnapshot(const sim::FrameSnapshot& snapshot, std::vector<std::byte>& bytes)
{
    UNISON_VERIFY(snapshot.physicsState.size() <= std::numeric_limits<std::uint32_t>::max());

    bytes.resize(kHeaderSize + sizeof(snapshot.frameNumber) + sizeof(snapshot.dt) + serializedSizeOf(snapshot.globals) +
                 sim::serializedSizeOf(snapshot.registry) + sizeof(std::uint32_t) + snapshot.physicsState.size());

    BinaryWriter writer{bytes};
    const bool isWritten = writer.writeValue(kSnapshotMagic) && writer.writeValue(kSnapshotVersion) &&
                           writer.writeValue(sim::componentRegistry().layoutHash()) &&
                           writer.writeValue(snapshot.frameNumber) && writer.writeValue(snapshot.dt) &&
                           writeGlobals(writer, snapshot.globals) && sim::writeRegistry(writer, snapshot.registry) &&
                           writer.writeValue(static_cast<std::uint32_t>(snapshot.physicsState.size())) &&
                           writer.writeBytes(snapshot.physicsState);

    UNISON_VERIFY(isWritten && writer.remaining() == 0);
}

tl::expected<void, Error> deserializeSnapshot(std::span<const std::byte> bytes, sim::FrameSnapshot& snapshot)
{
    BinaryReader reader{bytes};

    if (const tl::expected<void, Error> header = readHeader(reader); !header.has_value())
    {
        return header;
    }

    const std::optional<std::uint32_t> frameNumber = reader.readValue<std::uint32_t>();
    const std::optional<float> dt = frameNumber.has_value() ? reader.readValue<float>() : std::nullopt;
    const std::optional<sim::Globals> globals = dt.has_value() ? readGlobals(reader) : std::nullopt;

    if (!globals.has_value())
    {
        return malformed("the snapshot's globals end early or hold what no frame could");
    }

    if (!sim::readRegistry(reader, snapshot.registry))
    {
        return malformed("the snapshot's registry ends early or holds what no registry could");
    }

    const std::optional<std::uint32_t> physicsSize = reader.readValue<std::uint32_t>();
    const std::optional<std::span<const std::byte>> physicsState =
        physicsSize.has_value() ? reader.readBytes(*physicsSize) : std::nullopt;

    if (!physicsState.has_value() || reader.remaining() != 0)
    {
        return malformed("the snapshot's physics state ends early or bytes follow it");
    }

    snapshot.frameNumber = *frameNumber;
    snapshot.dt = *dt;
    snapshot.globals = *globals;
    snapshot.physicsState.assign(physicsState->begin(), physicsState->end());

    return {};
}

}
