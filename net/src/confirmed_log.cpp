#include <unison/net/confirmed_log.hpp>

#include <unison/core/contract.hpp>

namespace unison::net
{

ConfirmedLog::ConfirmedLog(std::size_t frameSize) : frameSize{frameSize}
{
}

void ConfirmedLog::append(std::span<const std::byte> slots)
{
    UNISON_VERIFY(slots.size() == frameSize);

    frames.insert(frames.end(), slots.begin(), slots.end());
}

std::uint32_t ConfirmedLog::lastFrame() const
{
    return frameSize == 0 ? 0 : static_cast<std::uint32_t>(frames.size() / frameSize);
}

std::span<const std::byte> ConfirmedLog::slotsOf(std::uint32_t firstFrame, std::uint32_t frameCount) const
{
    const bool isHeld = firstFrame >= 1 && frameCount <= lastFrame() && firstFrame <= lastFrame() - frameCount + 1;

    UNISON_VERIFY(isHeld);

    if (!isHeld)
    {
        return {};
    }

    return std::span{frames}.subspan(std::size_t{firstFrame - 1} * frameSize, std::size_t{frameCount} * frameSize);
}

}
