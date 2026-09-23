#pragma once

namespace unison::net
{

/// Asks the system for a timer of a millisecond for as long as it lives, so that a host loop sleeping a
/// millisecond between rounds wakes after about one, not after a tick of Windows' default 15.6 ms timer.
class MillisecondTimer
{
public:
    MillisecondTimer();
    ~MillisecondTimer();

    MillisecondTimer(const MillisecondTimer&) = delete;
    MillisecondTimer& operator=(const MillisecondTimer&) = delete;
    MillisecondTimer(MillisecondTimer&&) = delete;
    MillisecondTimer& operator=(MillisecondTimer&&) = delete;

private:
    bool isGranted = false;
};

}
