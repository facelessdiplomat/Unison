#pragma once

#include <unison/core/binary_reader.hpp>
#include <unison/core/error.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/replay_format.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <span>

namespace unison::session
{

/// Reads a replay from bytes it views, which must outlive it: the header as it opens, then a record at a time.
/// Bytes without the magic, another version of the format, a config no session can play, a record of no known
/// kind and a replay that ends inside its header or a record are refused.
class ReplayReader
{
public:
    [[nodiscard]] static tl::expected<ReplayReader, Error> open(std::span<const std::byte> bytes);

    [[nodiscard]] const net::SessionConfig& config() const;

    /// Whether every record has been read.
    [[nodiscard]] bool isAtEnd() const;

    /// The next record, in the order it was written.
    [[nodiscard]] tl::expected<ReplayRecord, Error> next();

private:
    ReplayReader(const BinaryReader& reader, const net::SessionConfig& config);

    [[nodiscard]] tl::expected<ReplayRecord, Error> nextFrame();

    [[nodiscard]] tl::expected<ReplayRecord, Error> nextChecksum();

    BinaryReader reader;
    net::SessionConfig recordedConfig;
};

}
