#pragma once

#include <cstdint>
#include <string_view>

namespace unison
{

/// Kind of a recoverable failure, for callers that branch on what went wrong. It gains a value when
/// a boundary gains a failure it can actually produce, never before.
enum class ErrorCode : std::uint16_t
{
    TruncatedInput,
    BufferTooSmall,
    MalformedMessage,
    InvalidOption,
    NetworkUnavailable,
    MalformedReplay,
    UnsupportedReplayVersion,
    FileUnavailable,
    ForeignReplay,
};

/// A recoverable failure returned from a boundary inside tl::expected. It is small and trivially
/// copyable so returning one costs nothing, and its message must outlive it, which a string literal
/// does. Simulation code never produces one: an invalid simulation state is a contract violation.
class Error
{
public:
    constexpr Error(ErrorCode code, std::string_view message) : failureCode{code}, failureMessage{message}
    {
    }

    [[nodiscard]] constexpr ErrorCode code() const
    {
        return failureCode;
    }

    [[nodiscard]] constexpr std::string_view message() const
    {
        return failureMessage;
    }

private:
    ErrorCode failureCode;
    std::string_view failureMessage;
};

}
