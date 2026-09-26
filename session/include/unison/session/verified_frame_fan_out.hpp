#pragma once

#include <unison/session/verified_frame_receiver.hpp>

#include <vector>

namespace unison::session
{

/// Hands every frame a session verifies to each of several receivers in turn, so one session can feed a replay and a
/// desync dumper at once. The receivers must outlive it.
class VerifiedFrameFanOut final : public IVerifiedFrameReceiver
{
public:
    explicit VerifiedFrameFanOut(std::vector<IVerifiedFrameReceiver*> receivers);

    void frameVerified(const VerifiedFrame& frame) override;

private:
    std::vector<IVerifiedFrameReceiver*> receivers;
};

}
