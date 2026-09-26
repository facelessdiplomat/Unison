#pragma once

#include <unison/core/error.hpp>

#include <tl/expected.hpp>

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace unison::session
{

/// Writes a replay's bytes into a file, replacing what it held; a file that cannot be written whole is reported.
[[nodiscard]] tl::expected<void, Error> writeReplayFile(const std::filesystem::path& path,
                                                        std::span<const std::byte> bytes);

/// Reads a replay file's bytes whole; a file that cannot be read whole is reported.
[[nodiscard]] tl::expected<std::vector<std::byte>, Error> readReplayFile(const std::filesystem::path& path);

}
