#pragma once

#include <string_view>

namespace unison
{

/// Callback a host installs to end the process its own way when an invariant breaks. It is expected
/// not to return; the default handler aborts. A handler that does return lets execution carry on in
/// a state the engine has already declared invalid, which only a test should ever want.
using FatalHandler = void (*)(std::string_view message);

/// Installs the process-wide fatal handler, replacing the previous one; a null handler restores the
/// default, which aborts.
void installFatalHandler(FatalHandler handler);

/// Returns the process-wide fatal handler.
[[nodiscard]] FatalHandler installedFatalHandler();

/// Reports a broken invariant through LogSink at error level, then calls the fatal handler. Called
/// by UNISON_VERIFY; there is no reason to call it directly.
void reportBrokenContract(std::string_view condition, std::string_view file, int line);

}

/// Checks an invariant that must hold in every build. On failure it reports the condition and its
/// place through LogSink, then calls the fatal handler.
#define UNISON_VERIFY(condition)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            ::unison::reportBrokenContract(#condition, __FILE__, __LINE__);                                            \
        }                                                                                                              \
    } while (false)

/// Checks a programmer contract in Debug only. The condition is not evaluated in Release, so it must
/// have no side effects or Debug and Release stop behaving alike.
#ifdef NDEBUG
    #define UNISON_ASSERT(condition) ((void)0)
#else
    #define UNISON_ASSERT(condition) UNISON_VERIFY(condition)
#endif
