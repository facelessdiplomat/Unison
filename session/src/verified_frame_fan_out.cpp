#include <unison/session/verified_frame_fan_out.hpp>

#include <utility>

namespace unison::session
{

VerifiedFrameFanOut::VerifiedFrameFanOut(std::vector<IVerifiedFrameReceiver*> receivers)
    : receivers{std::move(receivers)}
{
}

void VerifiedFrameFanOut::frameVerified(const VerifiedFrame& frame)
{
    for (IVerifiedFrameReceiver* receiver : receivers)
    {
        receiver->frameVerified(frame);
    }
}

}
