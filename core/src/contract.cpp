#include <unison/core/contract.hpp>

#include <unison/core/log_sink.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>

namespace unison
{

namespace
{

void abortOnBrokenContract(std::string_view)
{
    std::abort();
}

std::atomic<FatalHandler> installedHandler{&abortOnBrokenContract};

std::size_t append(std::span<char> buffer, std::size_t position, std::string_view text)
{
    const std::size_t count = std::min(text.size(), buffer.size() - position);

    std::memcpy(buffer.data() + position, text.data(), count);

    return position + count;
}

std::size_t appendNumber(std::span<char> buffer, std::size_t position, int value)
{
    std::array<char, 16> digits{};
    const std::to_chars_result written = std::to_chars(digits.data(), digits.data() + digits.size(), value);

    if (written.ec != std::errc{})
    {
        return position;
    }

    return append(
        buffer, position, std::string_view{digits.data(), static_cast<std::size_t>(written.ptr - digits.data())});
}

}

void installFatalHandler(FatalHandler handler)
{
    installedHandler.store(handler != nullptr ? handler : &abortOnBrokenContract, std::memory_order_relaxed);
}

FatalHandler installedFatalHandler()
{
    return installedHandler.load(std::memory_order_relaxed);
}

void reportBrokenContract(std::string_view condition, std::string_view file, int line)
{
    std::array<char, 512> storage{};
    std::size_t position = 0;

    position = append(storage, position, "broken contract: ");
    position = append(storage, position, condition);
    position = append(storage, position, " at ");
    position = append(storage, position, file);
    position = append(storage, position, ":");
    position = appendNumber(storage, position, line);

    const std::string_view message{storage.data(), position};

    logMessage(LogLevel::Error, message);
    installedHandler.load(std::memory_order_relaxed)(message);
}

}
