#pragma once

#include <unison/core/error.hpp>
#include <unison/net/session_config.hpp>
#include <unison/session/replay_verification.hpp>

#include <tl/expected.hpp>

#include <cstdint>
#include <string>

namespace unison::replay
{

/// Refuses a replay whose config names other assets or systems than the build plays with, whose hashes are given.
[[nodiscard]] tl::expected<void, Error>
checkRecordedContent(const net::SessionConfig& recorded, std::uint64_t assetHash, std::uint64_t pipelineHash);

/// The exit code verifying ends with: nought when every checksum compared matched, two when one did not.
[[nodiscard]] int verifyExitCodeOf(const session::ReplayVerdict& verdict);

/// What verifying prints: how many of the checksums compared match, over how many frames, and the first frame that
/// differs.
[[nodiscard]] std::string verifyReportOf(const session::ReplayVerdict& verdict);

/// What playing prints: how many frames were played and the checksum of the last one.
[[nodiscard]] std::string playReportOf(const session::ReplayVerdict& verdict, std::uint64_t lastChecksum);

}
