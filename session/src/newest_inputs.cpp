#include <unison/session/newest_inputs.hpp>

#include <unison/core/contract.hpp>

#include <algorithm>

namespace unison::session
{

std::optional<net::Input> newestInputsOf(const Session& session,
                                         std::uint8_t slot,
                                         std::uint32_t inputDelay,
                                         std::uint8_t inputSize,
                                         std::span<std::byte> batch)
{
    UNISON_VERIFY(batch.size() >= std::size_t{kRedundantInputs} * inputSize);

    const std::uint32_t newest = session.predictedFrame() + inputDelay;
    const std::uint32_t oldestRepeated = newest < kRedundantInputs ? 1U : newest + 1U - kRedundantInputs;
    const std::uint32_t oldest = std::max(session.verifiedFrame() + 1U, oldestRepeated);

    if (newest < oldest || batch.size() < std::size_t{kRedundantInputs} * inputSize)
    {
        return std::nullopt;
    }

    const std::uint32_t count = newest - oldest + 1U;
    const std::span<std::byte> written = batch.first(std::size_t{count} * inputSize);

    for (std::uint32_t offset = 0; offset < count; ++offset)
    {
        std::ranges::copy(session.inputs().inputsAt(oldest + offset).bytesAt(slot).first(inputSize),
                          written.subspan(std::size_t{offset} * inputSize).begin());
    }

    return net::Input{oldest, inputSize, static_cast<std::uint8_t>(count), written};
}

}
