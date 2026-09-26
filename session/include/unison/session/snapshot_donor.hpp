#pragma once

#include <unison/net/outbox.hpp>
#include <unison/net/transport.hpp>
#include <unison/session/verified_frame_receiver.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace unison::session
{

/// Answers the relay's request for a snapshot: remembers the frame asked for and, when its session verifies the first
/// frame at or after it, sends the relay that frame's serialised snapshot in chunks over the reliable channel.
class SnapshotDonor final : public IVerifiedFrameReceiver
{
public:
    SnapshotDonor(net::Outbox& outbox, net::PeerId relay);

    /// Asks for the snapshot of the first frame verified at or after this one, in place of any asked for before.
    void request(std::uint32_t frame);

    void frameVerified(const VerifiedFrame& frame) override;

private:
    net::Outbox& outbox;
    net::PeerId relay;
    std::optional<std::uint32_t> requested;
    std::vector<std::byte> serialized;
};

}
