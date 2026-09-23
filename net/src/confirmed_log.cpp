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

std::span<const std::byte> ConfirmedLog::slotsOf(std::uint32_t frame) const
{
    UNISON_VERIFY(frame >= 1 && frame <= lastFrame());

    if (frame < 1 || frame > lastFrame())
    {
        return {};
    }

    return std::span{frames}.subspan(std::size_t{frame - 1} * frameSize, frameSize);
}

}
