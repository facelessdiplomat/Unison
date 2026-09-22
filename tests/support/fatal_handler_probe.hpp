#pragma once

#include <unison/core/contract.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace unison::test
{

namespace detail
{

inline std::size_t brokenContractCount = 0;
inline std::string lastBrokenContract;

inline void recordBrokenContract(std::string_view message)
{
    ++brokenContractCount;
    lastBrokenContract = std::string{message};
}

}

/// Catches broken contracts for the length of a scope instead of letting them end the process, so a
/// test can assert that one was reported. The previous handler is restored on exit.
class FatalHandlerProbe
{
public:
    FatalHandlerProbe() : previousHandler{installedFatalHandler()}
    {
        detail::brokenContractCount = 0;
        detail::lastBrokenContract.clear();
        installFatalHandler(&detail::recordBrokenContract);
    }

    ~FatalHandlerProbe()
    {
        installFatalHandler(previousHandler);
    }

    FatalHandlerProbe(const FatalHandlerProbe&) = delete;
    FatalHandlerProbe& operator=(const FatalHandlerProbe&) = delete;
    FatalHandlerProbe(FatalHandlerProbe&&) = delete;
    FatalHandlerProbe& operator=(FatalHandlerProbe&&) = delete;

    [[nodiscard]] std::size_t failureCount() const
    {
        return detail::brokenContractCount;
    }

    [[nodiscard]] std::string_view lastMessage() const
    {
        return detail::lastBrokenContract;
    }

private:
    FatalHandler previousHandler;
};

}
