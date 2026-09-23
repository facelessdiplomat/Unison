#pragma once

#include <cstdint>

namespace unison::net
{

/// Tells the time in microseconds since a start of its own. A host puts a real clock behind it, while tests
/// and the runner move a manual one on by hand, so whoever needs the time is given it rather than reading it.
class IClock
{
public:
    IClock() = default;
    virtual ~IClock() = default;

    IClock(const IClock&) = delete;
    IClock& operator=(const IClock&) = delete;
    IClock(IClock&&) = delete;
    IClock& operator=(IClock&&) = delete;

    [[nodiscard]] virtual std::uint64_t nowMicroseconds() const = 0;
};

/// A clock that stands still until it is moved on.
class ManualClock final : public IClock
{
public:
    [[nodiscard]] std::uint64_t nowMicroseconds() const override;

    void advance(std::uint64_t microseconds);

private:
    std::uint64_t now = 0;
};

}
