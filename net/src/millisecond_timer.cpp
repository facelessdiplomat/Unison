#include <unison/net/millisecond_timer.hpp>

#ifdef _WIN32
    #include <windows.h>

    #include <timeapi.h>
#endif

namespace unison::net
{

namespace
{

#ifdef _WIN32
constexpr UINT kOneMillisecond = 1;

bool askForMillisecondTimer()
{
    return timeBeginPeriod(kOneMillisecond) == TIMERR_NOERROR;
}

void giveBackMillisecondTimer()
{
    static_cast<void>(timeEndPeriod(kOneMillisecond));
}
#else
bool askForMillisecondTimer()
{
    return false;
}

void giveBackMillisecondTimer()
{
}
#endif

}

MillisecondTimer::MillisecondTimer() : isGranted{askForMillisecondTimer()}
{
}

MillisecondTimer::~MillisecondTimer()
{
    if (isGranted)
    {
        giveBackMillisecondTimer();
    }
}

}
